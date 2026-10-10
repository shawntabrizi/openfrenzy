/*
TankedUp/CTank_Camera.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Camera.cpp

CTank_Camera (CTank_Game +0xd40, "cameraSystem"): a list of camera shots read from the
variation's CAMERAFILE ("CAMERA { ... }" blocks). The game state picks a shot by name
("INTO CAM", "LINK TO START CAM", "COUNTDOWN CAM", "GAME CAM", "EXIT CAM", "DONE CAM",
"FADED CAM"); a shot can chain to the next one after END_TIME, follow entities (the
players, the winners), orbit, zoom out until every tank is on screen, or play a LOM
intro path (classlib_ui CLIntroCam). It also darkens the screen for the intro and the end,
and has a debug "filmCam" moved with pad 1.

0x001c5030 and 0x001c6a70 are tagged with this file; the functions from 0x001c4850 to
0x001c71c0 are placed here because of their "CTank_Camera::" strings or because they work
on this layout.
*/

#include "tankedup_a.h"

/* a camera shot (0x14c bytes, from the heap) */
struct TuShot {
	char name[0x40];             /* +0x000 NAME */
	void *menu;                  /* +0x040 tweak menu of the shot */
	void *fromMenu;              /* +0x044 "look from" */
	void *toMenu;                /* +0x048 "look to" */
	int fromType;                /* +0x04c FROM_TYPE: 0 fixed, 1 entity, 2 previous shot's */
	int toType;                  /* +0x050 TO_TYPE: 0 fixed, 1 tanks' centre, 2 entity, 3 previous shot's */
	int zoomType;                /* +0x054 ZOOM_TYPE: 0 none, 1 fit the tanks, 2 previous shot's */
	float from[4];               /* +0x058 the eye before zooming */
	float eye[4];                /* +0x068 */
	float to[4];                 /* +0x078 the target */
	char entName[0x40];          /* +0x088 ENT_NAME ("<WINNER> " = the winners in turn) */
	int flags;                   /* +0x0c8 FLAGS (bit 0: orbit) */
	char lomFile[0x40];          /* +0x0cc LOM: an intro path file */
	float fromLoc[4];            /* +0x10c FROM_LOC (or the offset from the entity) */
	float orbitRadius;           /* +0x11c ORBIT */
	float orbitTime;             /* +0x120 ORBIT_SPEED (seconds per turn) */
	float toLoc[4];              /* +0x124 TO_LOC */
	float toLerpSpeed;           /* +0x134 TO_LERP_SPEED */
	float zoom;                  /* +0x138 0 = from, 1 = target */
	float time;                  /* +0x13c frames in this shot */
	float endTime;               /* +0x140 END_TIME (frames; 0 = never) */
	float linkTime;              /* +0x144 LINK_TIME (frames of blend from the previous shot) */
	struct TuShot *next;         /* +0x148 */
};
OF_ASSERT_SIZE(struct TuShot, 0x14c);

/* the camera system (CTank_Game +0xd40, 0xf0 bytes) */
struct TuCamera {
	int winners[4];              /* +0x00 entity ids of the winners, in order */
	int winnerCount;             /* +0x10 */
	int state;                   /* +0x14 the game state */
	int fps;                     /* +0x18 frames in the last second (debug) */
	int unknown1c;               /* +0x1c */
	unsigned char timer[0x40];   /* +0x20 a timer (struct TuTimer without its last word) */
	int second;                  /* +0x60 the timer's last whole second */
	int framesThisSecond;        /* +0x64 */
	int framesLastSecond;        /* +0x68 */
	struct TuShot *shot;         /* +0x6c the current shot */
	int fade;                    /* +0x70 the intro's darkening (255 -> 0) */
	int paused;                  /* +0x74 */
	void *controller;            /* +0x78 the debug BCamController */
	void *entities;              /* +0x7c the entity system */
	int debugMode;               /* +0x80 DEBUGMODE */
	float unknown84;             /* +0x84 25 */
	struct TuShot *filmCam;      /* +0x88 the debug shot */
	struct TuShot *previous;     /* +0x8c the shot before the current one */
	struct TuShot *shots;        /* +0x90 the list */
	struct TuShot *fpCam;        /* +0x94 the shot named "FPCAM..." (single player view) */
	int blending;                /* +0x98 a blend from the previous shot ran this frame */
	float totalTime;             /* +0x9c frames since the start (debug) */
	int frames;                  /* +0xa0 frames this second (debug) */
	int totalFrames;             /* +0xa4 */
	int shotChanged;             /* +0xa8 */
	void *package;               /* +0xac */
	int fading;                  /* +0xb0 a CLFade runs */
	float step;                  /* +0xb4 the last frame step (for the fade) */
	int winner;                  /* +0xb8 the winner shown (-1 none) */
	void *input;                 /* +0xbc the debug input map */
	void *leftX;                 /* +0xc0 */
	void *leftY;                 /* +0xc4 */
	void *rightX;                /* +0xc8 */
	void *rightY;                /* +0xcc */
	void *primary;               /* +0xd0 */
	void *secondary;             /* +0xd4 */
	int lookCam;                 /* +0xd8 tweak switch: use the filmCam */
	float yaw;                   /* +0xdc filmCam angles and distance */
	float pitch;                 /* +0xe0 */
	float distance;              /* +0xe4 */
	int primaryCount;            /* +0xe8 */
	int secondaryCount;          /* +0xec */
};
OF_ASSERT_OFFSET(struct TuCamera, timer, 0x20);
OF_ASSERT_OFFSET(struct TuCamera, shot, 0x6c);
OF_ASSERT_OFFSET(struct TuCamera, input, 0xbc);
OF_ASSERT_SIZE(struct TuCamera, 0xf0);

/* an entity of tankedup_b (fields this file reads) */
#define TU_ENT_POS(e) TU_FP(e, 0xa08)       /* position (4 floats) */
#define TU_ENT_ROT_X(e) TU_F32(e, 0xa18)
#define TU_ENT_ROT_Y(e) TU_F32(e, 0xa1c)
#define TU_ENT_ROT_Z(e) TU_F32(e, 0xa20)

/* bInput channel fields: the value and its range */
#define TU_CHANNEL_VALUE(c) TU_I32(c, 0x14)
#define TU_CHANNEL_RANGE(c) TU_I32(c, 0x1c)

/* the debug tweak menu root of the debug object (0x003151b0) */
#define TU_DBG_MENU TU_VAR(void *, 0x003151b0)
/* the debug object's height of the point above a tank that must stay on screen */
#define TU_DBG_TANK_TOP TU_VAR(float, 0x00315178)
/* the debug object's height of the point that the "tanks' centre" target looks at */
#define TU_DBG_LOOK_DROP TU_VAR(float, 0x0031515c)
/* the debug object: the LOM intro path is skipped to its end when a shot changes */
#define TU_DBG_SKIP_LOM TU_VAR(int, 0x003150c8)

/* the view of the last SetView: the direction to the eye (0x0031532c) and the eye (0x0031533c) */
#define TU_VIEW_DIR (&TU_VAR(float, 0x0031532c))
#define TU_VIEW_EYE (&TU_VAR(float, 0x0031533c))

static void vec4_copy(float *dst, const float *src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

/* the position `offset` relative to entity `ent` (turned by its rotation) */
static void camera_entity_point(float *out, float *offset, void *ent)
{
	bmVectorRotateZXY(out, offset, TU_ENT_ROT_Z(ent), -TU_ENT_ROT_X(ent), TU_ENT_ROT_Y(ent));
}

/* 0x001c4850  TankedUp/CTank_Camera.cpp (inferred)
The constructor. */
void *CTankCamera_Ctor(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	CTankTimer_Ctor((struct TuTimer *)camera->timer);
	camera->controller = NULL;
	camera->debugMode = 0;
	camera->fpCam = NULL;
	camera->paused = 1;
	return self;
}

/* 0x001c4880  TankedUp/CTank_Camera.cpp (inferred)
The destructor (the timer's destructor is empty). */
void CTankCamera_Dtor(void *self)
{
	(void)self;
}

/* 0x001c4890  TankedUp/CTank_Camera.cpp (inferred)
Creates the debug input map on player 1's pad: both sticks and the two fire buttons. */
void CTankCamera_CreateInput(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	camera->input = biCreateMap(6);
	camera->leftY = biCreateChannel((struct biMap *)camera->input, "left Y", 3, 0, 0, 0, 0);
	camera->leftX = biCreateChannel((struct biMap *)camera->input, "left X", 3, 0, 0, 0, 0);
	camera->rightY = biCreateChannel((struct biMap *)camera->input, "right Y", 3, 0, 0, 0, 0);
	camera->rightX = biCreateChannel((struct biMap *)camera->input, "right X", 3, 0, 0, 0, 0);
	camera->primary = biCreateChannel((struct biMap *)camera->input, "primary fire", 1, 1, 0, 0, 0);
	camera->secondary = biCreateChannel((struct biMap *)camera->input, "secondary fire", 1, 1, 0, 0, 0);
	biBindInput((struct biChannel *)camera->leftY, DAT_002baf08 | 0xba, 0);
	biBindInput((struct biChannel *)camera->leftX, DAT_002baf08 | 0xb9, 0);
	biBindInput((struct biChannel *)camera->rightY, DAT_002baf08 | 0xbc, 0);
	biBindInput((struct biChannel *)camera->rightX, DAT_002baf08 | 0xbb, 0);
	biBindInput((struct biChannel *)camera->primary, DAT_002baf08 | 0xc5, 0);
	biBindInput((struct biChannel *)camera->secondary, DAT_002baf08 | 0xc6, 0);
}

/* 0x001c4a10  TankedUp/CTank_Camera.cpp (inferred)
Deletes the debug input map. */
void CTankCamera_ReleaseInput(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	if (camera->leftX != NULL)
		biDeleteChannel((struct biChannel *)camera->leftX);
	if (camera->leftY != NULL)
		biDeleteChannel((struct biChannel *)camera->leftY);
	if (camera->rightX != NULL)
		biDeleteChannel((struct biChannel *)camera->rightX);
	if (camera->rightY != NULL)
		biDeleteChannel((struct biChannel *)camera->rightY);
	if (camera->primary != NULL)
		biDeleteChannel((struct biChannel *)camera->primary);
	if (camera->secondary != NULL)
		biDeleteChannel((struct biChannel *)camera->secondary);
	camera->leftX = NULL;
	camera->leftY = NULL;
	camera->rightX = NULL;
	camera->rightY = NULL;
	camera->primary = NULL;
	camera->secondary = NULL;
	if (camera->input != NULL)
		biDeleteMap((struct biMap *)camera->input);
	camera->input = NULL;
}

/* 0x001c4ad0  TankedUp/CTank_Camera.cpp (inferred)
Starts the camera's timer. */
void CTankCamera_Start(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	CTankTimer_StartOrResume((struct TuTimer *)camera->timer);
	camera->frames = 0;
	camera->framesLastSecond = 0;
}

/* 0x001c4af0  TankedUp/CTank_Camera.cpp (inferred: "CTank_Camera::Initialise - Could not load
camera LOM file")
Starts the current shot's LOM intro path (classlib_ui CLIntroCam). Returns 0 on failure. */
int CTankCamera_LoadLom(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	unsigned int size;
	void *buffer;
	int *map;

	buffer = bkLoadFile((struct bkPackage *)camera->package, camera->shot->lomFile, NULL, &size,
		NULL, 0);
	if (buffer == NULL) {
		TU_DPRINTF("CTank_Camera::Initialise - Could not load camera LOM file [%s]\n",
			camera->shot->lomFile);
		return 0;
	}
	map = ParseMap_Load((char *)buffer, 2, (int)size);
	if (map == NULL) {
		TU_DPRINTF("CTank_Camera::Initialise - Error parsing camera LOM file [%s]\n",
			camera->shot->lomFile);
		return 0;
	}
	CLIntroCam_Load((int)map, (undefined4)self);
	ParseMap_Free(map);
	return 1;
}

/* 0x001c4b90  TankedUp/CTank_Camera.cpp (inferred)
Frames in the current shot. */
float CTankCamera_GetShotTime(void *self)
{
	return ((struct TuCamera *)self)->shot->time;
}

/* 0x001c4ba0  TankedUp/CTank_Camera.cpp (inferred)
Seconds left of the current shot. */
float CTankCamera_GetShotTimeLeft(void *self)
{
	struct TuShot *shot = ((struct TuCamera *)self)->shot;

	return (float)(((double)shot->endTime - (double)shot->time) * 0.0166666675f);
}

/* 0x001c4bc0  TankedUp/CTank_Camera.cpp (inferred)
After the scene: darkens the screen (CLScreen_DrawDarken) by state: "DONE CAM" fades to
black over the shot ("FADED CAM" stays black), the intro fades in over its first 60
frames, and a running CLFade is drawn. */
void CTankCamera_Draw(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *shot;

	if (camera->state == 6) {
		if (_stricmp(camera->shot->name, "FADED CAM") == 0) {
			CLScreen_DrawDarken(0xff, 1);
		} else {
			shot = camera->shot;
			CLScreen_DrawDarken(TU_FTOL((shot->endTime > 0.0f ? shot->time / shot->endTime : 0.0f)
				* 255.0f), 1);
		}
	}
	if (camera->state == 7)
		CLScreen_DrawDarken(0xff, 1);
	if (camera->state == 1) {
		shot = camera->shot;
		if (shot->time < 60.0f) {
			int level = TU_FTOL((60.0f - shot->time) * 255.0f * 0.0166666675f);

			if (camera->fade <= level)
				level = camera->fade;
			camera->fade = level;
			CLScreen_DrawDarken(level, 1);
		} else {
			CLScreen_DrawDarken(0, 1);
		}
	}
	if (camera->state == 3 || camera->state == 2)
		CLScreen_DrawDarken(0, 1);
	shot = camera->shot;
	if (DAT_002baeb8 != 0 && shot->time < 60.0f)
		CLScreen_DrawDarken(TU_FTOL((60.0f - shot->time) * 255.0f * 0.0166666675f), 1);
	else
		CLScreen_DrawDarken(0, 1);
	if (camera->fading != 0) {
		bdSetRenderState(3, 0, 0);
		bdSetWorldMatrix(DAT_002855f0);
		bdSetRenderState(6, 1, 0);
		baSetBrightness(0x100);
		bdSetTexture(0, NULL);
		bdSetWorldMatrix(DAT_002855f0);
		camera->fading = CLFade_Draw(camera->step) != 1;
	}
}

/* 0x001c4d60  TankedUp/CTank_Camera.cpp (inferred) */
void CTankCamera_Pause(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	if (camera->debugMode != 0) {
		void *controller = camera->controller;

		TU_DPRINTF("Camera:: Marked Pos [%f,%f,%f]\n", (double)TU_F32(controller, 0x68),
			(double)TU_F32(controller, 0x6c), (double)TU_F32(controller, 0x70));
		(void)controller;
	}
	camera->paused = 1;
}

/* 0x001c4da0  TankedUp/CTank_Camera.cpp (inferred) */
void CTankCamera_Resume(void *self)
{
	((struct TuCamera *)self)->paused = 0;
}

/* 0x001c4db0  TankedUp/CTank_Camera.cpp (inferred)
Debug input for the filmCam: the right stick moves it in and out, the left stick turns it;
primary fire picks the next player to look at; secondary fire picks a fixed corner
(every fifth press: follow the player again). Returns a value nobody reads. */
int CTankCamera_Input(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *film;
	float turned[4];
	int choice;

	if (TU_CHANNEL_VALUE(camera->primary) != 0)
		camera->primaryCount++;
	if (TU_CHANNEL_VALUE(camera->secondary) != 0)
		camera->secondaryCount++;
	camera->distance = camera->distance
		- (float)(-TU_CHANNEL_VALUE(camera->rightY)) / (float)TU_CHANNEL_RANGE(camera->rightY) * 10.0f;
	camera->yaw = (float)(-TU_CHANNEL_VALUE(camera->leftX)) / (float)TU_CHANNEL_RANGE(camera->leftX)
		* 0.05f + camera->yaw;
	camera->pitch = (float)(-TU_CHANNEL_VALUE(camera->leftY)) / (float)TU_CHANNEL_RANGE(camera->leftY)
		* 0.05f + camera->pitch;
	camera->yaw = CLMath_WrapAngle(camera->yaw);
	camera->pitch = CLMath_WrapAngle(camera->pitch);
	film = camera->filmCam;
	film->fromLoc[2] = camera->distance;
	film->fromLoc[0] = 0.0f;
	film->fromLoc[1] = 0.0f;
	film->fromLoc[3] = 1.0f;
	bmVectorRotateX(turned, camera->filmCam->fromLoc, camera->pitch);
	bmVectorRotateY(camera->filmCam->fromLoc, turned, camera->yaw);
	if (camera->filmCam->time > 2700.0f)
		camera->filmCam->time = 3.0f;
	sprintf(camera->filmCam->entName, "player%d", camera->primaryCount % 4 + 1);
	choice = camera->secondaryCount;
	if (choice % 5 != 0) {
		camera->filmCam->fromType = 0;
		choice = camera->secondaryCount % 5 - 1;
		film = camera->filmCam;
		switch (choice) {
		case 0:
			film->fromLoc[0] = 0.0f;
			film->fromLoc[1] = 1000.0f;
			film->fromLoc[2] = 0.0f;
			film->fromLoc[3] = 1.0f;
			break;
		case 1:
			film->fromLoc[0] = 1500.0f;
			film->fromLoc[1] = 1000.0f;
			film->fromLoc[2] = 1500.0f;
			film->fromLoc[3] = 1.0f;
			break;
		case 2:
			film->fromLoc[1] = 1000.0f;
			film->fromLoc[0] = 1500.0f;
			film->fromLoc[2] = 0.0f;
			film->fromLoc[3] = 1.0f;
			break;
		case 3:
			film->fromLoc[1] = 2000.0f;
			film->fromLoc[0] = 1500.0f;
			film->fromLoc[2] = 0.0f;
			film->fromLoc[3] = 1.0f;
			break;
		}
		return choice;
	}
	camera->filmCam->fromType = 1;
	return choice / 5;
}

/* 0x001c5030  TankedUp/CTank_Camera.cpp
Appends a new shot with the default values to the list; *out gets it. */
void CTankCamera_NewShot(void *self, struct TuShot **out)
{
	struct TuShot **link = &((struct TuCamera *)self)->shots;
	struct TuShot *shot;

	while (*link != NULL)
		link = &(*link)->next;
	shot = (struct TuShot *)TU_NEW(0x14c, TU_FILE_CAMERA, 0x137);
	*link = shot;
	*out = shot;
	shot->fromType = 0;
	shot->toType = 0;
	shot->zoomType = 0;
	shot->flags = 0;
	shot->lomFile[0] = 0;
	shot->from[0] = 0.0f;
	shot->from[1] = 0.0f;
	shot->from[2] = 0.0f;
	shot->from[3] = 1.0f;
	shot->eye[0] = 0.0f;
	shot->eye[1] = 0.0f;
	shot->eye[2] = 0.0f;
	shot->eye[3] = 1.0f;
	shot->to[0] = 0.0f;
	shot->to[1] = 0.0f;
	shot->to[2] = 0.0f;
	shot->to[3] = 1.0f;
	shot->entName[0] = 0;
	shot->name[0] = 0;
	shot->fromLoc[0] = 0.0f;
	shot->fromLoc[2] = 0.0f;
	shot->fromLoc[1] = 10000.0f;
	shot->fromLoc[3] = 1.0f;
	shot->orbitRadius = 0.0f;
	shot->orbitTime = 30.0f;
	shot->toLoc[0] = 0.0f;
	shot->toLoc[1] = 0.0f;
	shot->toLoc[2] = 0.0f;
	shot->toLoc[3] = 1.0f;
	shot->toLerpSpeed = 0.01f;
	shot->zoom = 0.0f;
	shot->time = 0.0f;
	shot->endTime = 0.0f;
	shot->linkTime = -1.0f;
	shot->next = NULL;
}

/* 0x001c5180  TankedUp/CTank_Camera.cpp (inferred: "CTank_Camera::LoadCameraInfo")
Reads the camera file: DEBUGMODE, and "CAMERA {" blocks (one shot each). Times are in
seconds in the file and frames in the shot. */
void CTankCamera_LoadCameraInfo(void *self, struct TuTextReader *reader)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *shot;
	char command[512];
	char line[512];
	char *text;
	int result;
	int i1;
	float f1, f2, f3;

	camera->unknown84 = 25.0f;
	while (reader->cur < reader->end) {
		text = TUB_Reader_ReadLine(reader, line);
		if (strncmp(text, "//", 2) == 0)
			continue;
		if (_stricmp(text, "DEBUGMODE") == 0) {
			TU_DPRINTF("CAMERA IS NOW IN DEBUG MODE\n");
			camera->debugMode = 1;
		}
		if (_stricmp(text, "CAMERA {") != 0)
			continue;
		shot = NULL;
		CTankCamera_NewShot(self, &shot);
		while (reader->cur < reader->end) {
			if (_stricmp(text, "}") == 0)
				break;
			text = TUB_Reader_ReadLine(reader, line);
			if (strncmp(text, "//", 2) == 0)
				continue;
			result = 0;
			if (_strnicmp(text, "FROM_TYPE ", 10) == 0) {
				result = 2;
				if (sscanf(text, "%*s%d", &i1) == 1) {
					shot->fromType = i1;
					result = 1;
				}
			}
			if (_strnicmp(text, "TO_TYPE ", 8) == 0) {
				result = 2;
				if (sscanf(text, "%*s%d", &i1) == 1) {
					shot->toType = i1;
					result = 1;
				}
			}
			if (_strnicmp(text, "ZOOM_TYPE ", 10) == 0) {
				result = 2;
				if (sscanf(text, "%*s%d", &i1) == 1) {
					shot->zoomType = i1;
					result = 1;
				}
			}
			if (_strnicmp(text, "FROM_LOC ", 9) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f%f%f", &f1, &f2, &f3) == 3) {
					shot->fromLoc[0] = f1;
					shot->fromLoc[1] = f2;
					shot->fromLoc[3] = 1.0f;
					result = 1;
					shot->fromLoc[2] = f3;
				}
			}
			if (_strnicmp(text, "TO_LOC ", 7) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f%f%f", &f1, &f2, &f3) == 3) {
					shot->toLoc[0] = f1;
					shot->toLoc[1] = f2;
					shot->toLoc[3] = 1.0f;
					result = 1;
					shot->toLoc[2] = f3;
				}
			}
			if (_strnicmp(text, "FLAGS ", 6) == 0) {
				result = 2;
				if (sscanf(text, "%*s%d", &i1) == 1) {
					shot->flags = i1;
					result = 1;
				}
			}
			if (_strnicmp(text, "LINK_TIME ", 10) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f", &f1) == 1) {
					shot->linkTime = f1 * 60.0f;
					result = 1;
				}
			}
			if (_strnicmp(text, "END_TIME ", 9) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f", &f1) == 1) {
					shot->endTime = f1 * 60.0f;
					result = 1;
				}
			}
			if (_strnicmp(text, "ORBIT ", 6) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f", &f1) == 1) {
					shot->orbitRadius = f1;
					result = 1;
				}
			}
			if (_strnicmp(text, "ORBIT_SPEED ", 0xc) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f", &f1) == 1) {
					shot->orbitTime = f1;
					result = 1;
				}
			}
			if (_strnicmp(text, "ENT_NAME ", 9) == 0) {
				result = 2;
				if (BCarText_GetName(text, shot->entName, 0x3f) != 0)
					result = 1;
			}
			if (_strnicmp(text, "NAME ", 5) == 0) {
				result = 2;
				if (BCarText_GetName(text, shot->name, 0x3f) != 0)
					result = 1;
				if (_strnicmp(shot->name, "FPCAM", 5) == 0)
					camera->fpCam = shot;
			}
			if (_strnicmp(text, "LOM ", 4) == 0) {
				result = 2;
				if (BCarText_GetName(text, shot->lomFile, 0x3f) != 0)
					result = 1;
			}
			if (_strnicmp(text, "TO_LERP_SPEED ", 0xe) == 0) {
				result = 2;
				if (sscanf(text, "%*s%f", &f1) == 1) {
					shot->toLerpSpeed = f1;
					result = 1;
				}
			}
			if (_stricmp(text, "}") != 0) {
				sscanf(text, "%s", command);
				if (result == 0)
					TU_DPRINTF("CTank_Camera::LoadCameraInfo(\"%s\", %d): Invalid Command \"%s\"\n",
						reader->name, reader->line, command);
				else if (result == 2)
					TU_DPRINTF("CTank_Camera::LoadCameraInfo(\"%s\", %d): Bad use of Command \"%s\"\n",
						reader->name, reader->line, command);
			}
		}
	}
}

/* 0x001c56e0  TankedUp/CTank_Camera.cpp (inferred)
Works out a shot's eye before zooming ("from"): fixed (FROM_LOC), relative to an entity
(ENT_NAME; "<WINNER> " walks through the winners over the shot), or the previous shot's.
An orbiting shot (FLAGS bit 0) circles that point. `step` is not used. */
void CTankCamera_UpdateFrom(void *self, struct TuShot *shot, float step)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	float point[4];
	void *ent;
	int index;

	(void)step;
	if (shot->fromType == 0)
		goto fixed;
	if (shot->fromType == 2) {
		vec4_copy(shot->from, camera->previous->from);
		goto orbit;
	}
	if (shot->fromType != 1)
		goto orbit;
	if (_strnicmp(shot->entName, "<WINNER> ", 8) == 0) {
		int count;

		if (camera->winnerCount == 0)
			camera->winnerCount = 1;
		count = camera->winnerCount;
		if (count == 1 && DAT_002bae98 == 1) {
			camera->shot->endTime = 240.0f;
			index = 0;
		} else {
			camera->shot->endTime = (float)count * 240.0f;
			index = TU_FTOL(shot->time / shot->endTime * (float)camera->winnerCount);
		}
		if (camera->winnerCount - 1 < index)
			index = camera->winnerCount - 1;
		ent = CTankEntities_Get(camera->entities, camera->winners[index]);
		if (ent == NULL) {
			vec4_copy(shot->from, shot->fromLoc);
		} else {
			camera_entity_point(shot->from, shot->fromLoc, ent);
			shot->from[0] = shot->from[0] + TU_ENT_POS(ent)[0];
			shot->from[1] = shot->from[1] + TU_ENT_POS(ent)[1];
			shot->from[2] = shot->from[2] + TU_ENT_POS(ent)[2];
		}
		goto orbit;
	}
	ent = CTankEntities_Get(camera->entities, CTankEntities_FindId(camera->entities, shot->entName));
	if (ent != NULL) {
		camera_entity_point(point, shot->fromLoc, ent);
		point[0] = point[0] + TU_ENT_POS(ent)[0];
		point[1] = point[1] + TU_ENT_POS(ent)[1];
		point[2] = point[2] + TU_ENT_POS(ent)[2];
		bmVectorLerp(shot->from, point, shot->from, 0.0f);
		goto orbit;
	}
fixed:
	vec4_copy(shot->from, shot->fromLoc);
orbit:
	if ((shot->flags & 1) != 0) {
		float time = shot->time < shot->linkTime ? shot->linkTime : shot->time;
		double angle = (double)time / ((double)shot->orbitTime * 60.0f) * 6.28318548f;

		shot->from[0] = (float)(cos(angle) * shot->orbitRadius + shot->from[0]);
		shot->from[2] = (float)(sin(angle) * shot->orbitRadius + shot->from[2]);
	}
}

/* 0x001c5950  TankedUp/CTank_Camera.cpp (inferred)
Works out a shot's target ("to"): fixed (TO_LOC), the centre of the tanks (eased with
TO_LERP_SPEED unless the shot snaps), relative to an entity (or the winners in turn), or
the previous shot's. A shot snaps while it blends in, during a blend, and in its first
frame. */
void CTankCamera_UpdateTo(void *self, struct TuShot *shot, float step)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	float centre[4];
	float point[4];
	float unused1[4];
	float unused2[4];
	void *ent;
	int snap;
	int index;

	snap = shot->time < shot->linkTime || camera->blending != 0 || shot->time == 0.0f;
	switch (shot->toType) {
	case 0:
		vec4_copy(shot->to, shot->toLoc);
		return;
	case 1:
		TUB_Entities_GetCentre(camera->entities, centre, unused1, unused2);
		centre[1] = centre[1] - TU_DBG_LOOK_DROP;
		if (!snap) {
			bmVectorLerp(shot->to, shot->to, centre, step * shot->toLerpSpeed);
			return;
		}
		vec4_copy(shot->to, centre);
		return;
	case 2:
		break;
	case 3:
		vec4_copy(shot->to, camera->previous->to);
		return;
	default:
		return;
	}
	if (_strnicmp(shot->entName, "<WINNER> ", 8) == 0) {
		int count = camera->winnerCount;

		index = TU_FTOL(shot->time / shot->endTime * (float)count);
		if (camera->winner != index) {
			camera->winner = index;
			camera->fading = 1;
		}
		if (count - 1 < camera->winner) {
			camera->winner = count - 1;
			camera->fading = 0;
		}
		ent = CTankEntities_Get(camera->entities, camera->winners[camera->winner]);
		if (ent != NULL) {
			camera_entity_point(shot->to, shot->toLoc, ent);
			cmb_vec3_add(shot->to, shot->to, TU_ENT_POS(ent));
			return;
		}
	} else {
		ent = CTankEntities_Get(camera->entities, CTankEntities_FindId(camera->entities, shot->entName));
		if (ent != NULL) {
			camera_entity_point(point, shot->toLoc, ent);
			cmb_vec3_add(point, point, TU_ENT_POS(ent));
			bmVectorLerp(shot->to, point, shot->to, 0.0f);
			return;
		}
	}
	Core_Vec4Copy(shot->to, shot->toLoc);
}

/* the largest distance from the screen centre of `point` (1 = the screen edge; the height
counts 1.2 times, 0.2 up) */
static float camera_screen_extent(float *point)
{
	float screen[4];
	float x, y;

	bdProjectPoints(screen, point, 1);
	x = fabsf(screen[0] / (float)(DAT_002ba928 / 2));
	y = fabsf(screen[1] / (float)(DAT_002ba92c / 2) * 1.2f - 0.2f);
	return x > y ? x : y;
}

/* 0x001c5c00  TankedUp/CTank_Camera.cpp (inferred)
Works out a shot's zoom (how far the eye moves from "from" towards the target) and the
eye. Zoom type 1 tries eyes until three points of every active tank (above it, its
position, 500 above it) lie on screen: it steps the zoom up to 100 times (always 100 times
while the shot snaps), and never zooms in past 0.7. */
void CTankCamera_UpdateZoom(void *self, struct TuShot *shot, float step)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	int tries = 0;
	int snap;

	snap = shot->time < shot->linkTime || camera->blending != 0 || shot->time == 0.0f;
	if (shot->zoomType == 0) {
		shot->zoom = 0.0f;
	} else if (shot->zoomType == 2) {
		shot->zoom = camera->previous->zoom;
	} else if (shot->zoomType == 1) {
		float worst;

		for (;;) {
			void **node;

			tries++;
			bmVectorLerp(shot->eye, shot->from, shot->to, shot->zoom);
			bdSetCameraLookAt(shot->eye, shot->to, 0.0f);
			bdUpdateView();
			bdSetWorldMatrix(DAT_002855f0);
			worst = 0.0f;
			for (node = *(void ***)camera->entities; node != NULL; node = (void **)node[1]) {
				void *ent = node[0];
				float offset[4];
				float point[4];
				float extent;

				if (TU_I32(TU_PTR(ent, 0x91c), 0x20) == 0)
					continue;
				offset[2] = TU_DBG_TANK_TOP;
				offset[0] = 0.0f;
				offset[1] = 0.0f;
				offset[3] = 1.0f;
				camera_entity_point(point, offset, ent);
				point[0] = point[0] + TU_ENT_POS(ent)[0];
				point[1] = point[1] + TU_ENT_POS(ent)[1];
				point[2] = point[2] + TU_ENT_POS(ent)[2];
				point[3] = 1.0f;
				extent = camera_screen_extent(point);
				if (worst <= extent)
					worst = extent;
				TU_ENT_POS(ent)[3] = 1.0f;
				extent = camera_screen_extent(TU_ENT_POS(ent));
				if (worst <= extent)
					worst = extent;
				point[0] = TU_ENT_POS(ent)[0];
				point[1] = TU_ENT_POS(ent)[1] + 500.0f;
				point[2] = TU_ENT_POS(ent)[2];
				point[3] = 1.0f;
				extent = camera_screen_extent(point);
				if (worst <= extent)
					worst = extent;
			}
			if (snap) {
				shot->zoom = shot->zoom - (worst - 0.75f) * 0.05f;
			} else {
				if (worst > 0.8f)
					shot->zoom = shot->zoom - (worst - 0.8f) * 0.02f * step;
				if (worst < 0.7f)
					shot->zoom = (0.7f - worst) * 0.04f * step + shot->zoom;
			}
			if (tries >= 100)
				break;
			if (!snap) {
				if (!(worst > 0.95))
					break;
				continue;
			}
			if (!(worst > 0.7f) && !(worst < 0.8f))
				break;
		}
		if (shot->zoom >= 0.7f)
			shot->zoom = 0.7f;
	}
	bmVectorLerp(shot->eye, shot->from, shot->to, shot->zoom);
}

/* 0x001c60c0  TankedUp/CTank_Camera.cpp (inferred)
The camera's update. Returns the wished game state: the highest of `state` and the
camera's state, or the state of a shot that just started ("INTO CAM" 1 ... "FADED CAM" 7);
4 while the debug camera or the countdown runs. A shot passes to the next one after its
END_TIME (in state 5 only once the commentary is idle); a LOM shot follows its intro path
and passes on when the path ends. A new shot blends in from the previous one over its
LINK_TIME. */
int CTankCamera_Update(void *self, float step, int state)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	int asked = state;
	int frames;
	int result;

	if (camera->lookCam != 0)
		camera->shot = camera->filmCam;
	frames = camera->frames + 1;
	camera->step = step;
	camera->frames = frames;
	camera->totalFrames++;
	if (TU_FTOL((step + camera->totalTime) * 0.0166666675f)
		!= TU_FTOL(camera->totalTime * 0.0166666675f)) {
		TU_DPRINTF("frames per second: %.1f.  ", (double)frames);
		TU_DPRINTF("Over all fps:      %.2f\n",
			(double)camera->totalFrames / (camera->totalTime * 0.0166666675f));
		camera->fps = camera->frames;
		camera->frames = 0;
	}
	camera->totalTime = step + camera->totalTime;
	CTankTimer_Tick((struct TuTimer *)camera->timer);
	camera->framesThisSecond++;
	if (TU_FTOL(TU_Timer_GetElapsed(camera->timer)) != camera->second) {
		camera->framesLastSecond = camera->framesThisSecond;
		camera->framesThisSecond = 0;
	}
	camera->second = TU_FTOL(TU_Timer_GetElapsed(camera->timer));
	result = state;
	if (result <= camera->state)
		result = camera->state;
	if (camera->debugMode != 0) {
		TU_VFN(camera->controller, 4, void (*)(void *, float))(camera->controller, 1.0f);
		baUpdateCamera();
		return 4;
	}
	if (result == 3 && CLHud_UpdateCountdown(DAT_002ecfac) != 0)
		return 4;
	camera->shot->time = step + camera->shot->time;
	if (camera->shotChanged != 0) {
		camera->winner = -1;
		camera->shotChanged = 0;
		if (_stricmp(camera->shot->name, "INTO CAM") == 0)
			result = 1;
		if (_stricmp(camera->shot->name, "LINK TO START CAM") == 0)
			result = 2;
		if (_stricmp(camera->shot->name, "COUNTDOWN CAM") == 0)
			result = 3;
		if (_stricmp(camera->shot->name, "GAME CAM") == 0)
			result = 4;
		if (_stricmp(camera->shot->name, "EXIT CAM") == 0)
			result = 5;
		if (_stricmp(camera->shot->name, "DONE CAM") == 0)
			result = 6;
		if (_stricmp(camera->shot->name, "FADED CAM") == 0)
			result = 7;
		camera->shot->time = 0.0f;
		if (camera->shot->lomFile[0] != 0)
			CTankCamera_LoadLom(self);
	}
	if (camera->shot->lomFile[0] == 0) {
		struct TuShot *shot = camera->shot;

		if (shot->endTime != 0.0f && shot->next != NULL && shot->time > shot->endTime
			&& (asked != 5 || CLSamples_CommentaryIdle() != 0)) {
			camera->previous = shot;
			camera->shot = shot->next;
			Core_Vec4Copy(camera->shot->eye, shot->eye);
			Core_Vec4Copy(camera->shot->to, camera->previous->to);
			camera->shot->time = 0.0f;
			camera->shotChanged = 1;
			if (_stricmp(camera->shot->name, "EXIT CAM") == 0)
				camera->fading = 1;
			return result;
		}
		CTankCamera_UpdateTo(self, camera->shot, step);
		CTankCamera_UpdateFrom(self, camera->shot, step);
		CTankCamera_UpdateZoom(self, camera->shot, step);
		if (asked == 4) {
			int player;

			if (TUB_Debug_SinglePlayer(&player) != 0 && camera->fpCam != NULL) {
				sprintf(camera->fpCam->entName, "player%d", player + 1);
				CTankCamera_UpdateTo(self, camera->fpCam, step);
				CTankCamera_UpdateFrom(self, camera->fpCam, step);
				CTankCamera_UpdateZoom(self, camera->fpCam, step);
			}
		}
		camera->blending = 0;
		if (camera->previous != NULL && camera->shot->time < camera->shot->linkTime) {
			struct TuShot *cur = camera->shot;
			struct TuShot *prev;
			float t = cur->time / cur->linkTime;
			float a[4], b[4];

			bmVectorLerp(cur->eye, camera->previous->eye, cur->eye, t);
			prev = camera->previous;
			a[0] = prev->to[0] - prev->eye[0];
			a[1] = prev->to[1] - prev->eye[1];
			a[2] = prev->to[2] - prev->eye[2];
			cur = camera->shot;
			b[0] = cur->to[0] - cur->eye[0];
			b[1] = cur->to[1] - cur->eye[1];
			b[2] = cur->to[2] - cur->eye[2];
			bmVectorNormalise(a, a);
			bmVectorNormalise(b, b);
			bmVectorLerp(a, a, b, t);
			cur = camera->shot;
			cur->to[0] = a[0] + cur->eye[0];
			cur->to[1] = a[1] + cur->eye[1];
			cur->to[2] = a[2] + cur->eye[2];
			camera->blending = 1;
		}
		return result;
	}
	{
		int done = CLIntroCam_Update(step, 0);
		struct TuShot *shot;

		CLIntroCam_GetPosition(camera->shot->eye, CLIntroCam_GetTime());
		CLIntroCam_GetTarget(camera->shot->to, CLIntroCam_GetTime());
		if (done == -1) {
			TU_DPRINTF("***CAMERA ERROR***\n");
			return result;
		}
		if (done != 1)
			return result;
		CLIntroCam_Stop();
		CLIntroCam_GetPosition(camera->shot->eye, (float)CLIntroCam_GetFrameCount());
		CLIntroCam_GetTarget(camera->shot->to, (float)CLIntroCam_GetFrameCount());
		shot = camera->shot;
		vec4_copy(shot->from, shot->eye);
		camera->shot->zoom = 0.0f;
		camera->previous = camera->shot;
		camera->shot = camera->previous->next;
		vec4_copy(camera->shot->eye, camera->previous->eye);
		vec4_copy(camera->shot->to, camera->previous->to);
		camera->shotChanged = done;
		CLIntroCam_Destroy();
		return result;
	}
}

/* 0x001c66f0  TankedUp/CTank_Camera.cpp (inferred)
The heading of the view (from the eye before zooming to the target), of the single
player's own shot when there is one in state 4. */
float CTankCamera_GetHeading(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *shot;
	float dir[3];

	if (camera->state == 4 && TUB_Debug_SinglePlayer(NULL) != 0 && camera->fpCam != NULL)
		shot = camera->fpCam;
	else
		shot = camera->shot;
	dir[0] = shot->to[0] - shot->from[0];
	dir[1] = shot->to[1] - shot->from[1];
	dir[2] = shot->to[2] - shot->from[2];
	return CLMath_HeadingXZ(dir);
}

/* 0x001c6750  TankedUp/CTank_Camera.cpp (inferred)
Changes to the shot called `name` (the new shot starts where the old one looked; a LOM
path that runs is stopped, or skipped to its end with the debug option). Returns 0 when
there is no such shot; 1 in debug mode. */
int CTankCamera_SetShot(void *self, const char *name)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *shot;

	if (camera->debugMode != 0)
		return 1;
	shot = camera->shots;
	if (shot == NULL)
		return 0;
	while (_stricmp(shot->name, name) != 0) {
		shot = shot->next;
		if (shot == NULL)
			return 0;
	}
	if (shot != camera->shot) {
		camera->previous = camera->shot;
		camera->shotChanged = 1;
		camera->shot = shot;
		shot->time = 0.0f;
		if (camera->previous->lomFile[0] != 0) {
			if (TU_DBG_SKIP_LOM != 0) {
				CLIntroCam_Stop();
				CLIntroCam_GetPosition(camera->previous->eye, (float)CLIntroCam_GetFrameCount());
				CLIntroCam_GetTarget(camera->previous->to, (float)CLIntroCam_GetFrameCount());
				vec4_copy(camera->previous->from, camera->previous->eye);
				camera->previous->zoom = 0.0f;
			}
			CLIntroCam_Destroy();
		}
		vec4_copy(camera->shot->eye, camera->previous->eye);
		vec4_copy(camera->shot->to, camera->previous->to);
	}
	return 1;
}

/* 0x001c68c0  TankedUp/CTank_Camera.cpp (inferred)
Sets the view for the frame: the single player's own shot (updated here while paused) or
the current shot. Keeps the eye and the direction to it (0x0031533c, 0x0031532c), then sets
the render states for the scene. */
void CTankCamera_SetView(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuShot *shot;
	float dir[4];
	float eye[3];
	int player;

	if (camera->state == 4 && TUB_Debug_SinglePlayer(&player) != 0 && camera->fpCam != NULL) {
		if (camera->paused != 0) {
			sprintf(camera->fpCam->entName, "player%d", player + 1);
			CTankCamera_UpdateTo(self, camera->fpCam, 1.0f);
			CTankCamera_UpdateFrom(self, camera->fpCam, 1.0f);
			CTankCamera_UpdateZoom(self, camera->fpCam, 1.0f);
		}
		shot = camera->fpCam;
	} else {
		shot = camera->shot;
	}
	bdSetCameraLookAt(shot->eye, shot->to, 0.0f);
	bdUpdateView();
	shot = camera->shot;
	eye[0] = shot->eye[0];
	eye[1] = shot->eye[1];
	eye[2] = shot->eye[2];
	dir[0] = eye[0] - shot->to[0];
	dir[1] = eye[1] - shot->to[1];
	dir[2] = eye[2] - shot->to[2];
	dir[3] = 1.0f;
	bmVectorNormalise(dir, dir);
	TU_VIEW_EYE[0] = eye[0];
	TU_VIEW_EYE[1] = eye[1];
	TU_VIEW_EYE[2] = eye[2];
	TU_VIEW_DIR[0] = dir[0];
	TU_VIEW_EYE[3] = 1.0f;
	TU_VIEW_DIR[1] = dir[1];
	TU_VIEW_DIR[2] = dir[2];
	TU_VIEW_DIR[3] = dir[3];
	bdSetProjectionMode(1);
	bdSetRenderState(0, 1, 0);
	bdSetRenderState(1, 2, 0);
	bdSetRenderState(3, 1, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	baSetBrightness(0x100);
}

/* 0x001c6a70  TankedUp/CTank_Camera.cpp
Initialise: reads the camera file `file` from `package`, makes the debug controller
(DEBUGMODE), a tweak menu per shot, and the debug filmCam. Returns 0 when there is no
camera file or no shot. */
int CTankCamera_Initialise(void *self, void *package, char *file, void *entities)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	struct TuTextReader reader;
	struct TuShot *shot;
	struct TuShot *film;
	unsigned int size;
	void *buffer;

	camera->distance = 500.0f;
	camera->yaw = 0.0f;
	camera->pitch = 0.0f;
	camera->secondaryCount = 0;
	TUB_Reader_Ctor(&reader);
	camera->fade = 0xff;
	camera->paused = 1;
	camera->fps = 0;
	CTankTimer_Reset((struct TuTimer *)camera->timer);
	camera->entities = entities;
	camera->package = package;
	camera->frames = 0;
	camera->totalFrames = 0;
	camera->totalTime = 0.0f;
	camera->controller = NULL;
	camera->debugMode = 0;
	camera->fading = 0;
	camera->fpCam = NULL;
	if (file[0] == 0) {
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	buffer = bkLoadFile((struct bkPackage *)package, file, NULL, &size, NULL, 0);
	if (buffer == NULL) {
		TU_DPRINTF("CTank_Camera::Initialise - Configuration file [%s] not found\n", file);
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)buffer, (int)size, file);
	camera->shots = NULL;
	CTankCamera_LoadCameraInfo(self, &reader);
	camera->shot = camera->shots;
	camera->previous = NULL;
	camera->blending = 0;
	camera->winnerCount = 0;
	camera->winners[0] = 0;
	camera->winners[1] = 0;
	camera->winners[2] = 0;
	camera->winners[3] = 0;
	camera->winner = -1;
	if (camera->shots == NULL) {
		TUB_Reader_Dtor(&reader);
		return 0;
	}
	camera->shotChanged = 1;
	if (camera->debugMode != 0) {
		void *controller = TU_NEW(0xa8, TU_FILE_CAMERA, 0xdb);

		if (controller != NULL)
			controller = bCamController_Construct((struct BCamController *)controller);
		camera->controller = controller;
		bCamController_BindInput((struct BCamController *)controller, 0);
		bCamController_Setup((struct BCamController *)camera->controller, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 50.0f, 1);
		TU_F32(camera->controller, 0x54) = 1.0f;
		TU_F32(camera->controller, 0x58) = 80000.0f;
		TU_VFN(camera->controller, 8, void (*)(void *))(camera->controller);
	}
	camera->state = 1;
	camera->lookCam = 0;
	CTweakMenu_AddBool((struct CTweakMenu *)TU_DBG_MENU, "lookCam", &camera->lookCam);
	for (shot = camera->shots; shot != NULL; shot = shot->next) {
		void *menu = TU_NEW(0xe8, TU_FILE_CAMERA, 0xeb);

		if (menu != NULL)
			menu = CTweakMenu_CtorSub((struct CTweakMenu *)menu, (struct CTweakMenu *)TU_DBG_MENU,
				shot->name);
		shot->menu = menu;
		CTweakMenu_AddInt((struct CTweakMenu *)menu, "From Type", &shot->fromType, 1, 0, 2, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)shot->menu, "To Type", &shot->toType, 1, 0, 3, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)shot->menu, "Zoom Type", &shot->zoomType, 1, 0, 2, 0);
		CTweakMenu_AddInt((struct CTweakMenu *)shot->menu, "Orbit Type", &shot->flags, 1, 0, 1, 0);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->menu, "Orbit Radius", &shot->orbitRadius,
			10.0f, 0.0f, 10000.0f);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->menu, "Orbit Speed", &shot->orbitTime,
			1.0f, 1.0f, 1000.0f);
		menu = TU_NEW(0xe8, TU_FILE_CAMERA, 0xf2);
		if (menu != NULL)
			menu = CTweakMenu_CtorSub((struct CTweakMenu *)menu, (struct CTweakMenu *)shot->menu,
				"look from");
		shot->fromMenu = menu;
		CTweakMenu_AddFloat((struct CTweakMenu *)menu, "X", &shot->fromLoc[0], 10.0f, -10000.0f,
			10000.0f);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->fromMenu, "Y", &shot->fromLoc[1], 10.0f,
			-10000.0f, 10000.0f);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->fromMenu, "Z", &shot->fromLoc[2], 10.0f,
			-10000.0f, 10000.0f);
		menu = TU_NEW(0xe8, TU_FILE_CAMERA, 0xf6);
		if (menu != NULL)
			menu = CTweakMenu_CtorSub((struct CTweakMenu *)menu, (struct CTweakMenu *)shot->menu,
				"look to");
		shot->toMenu = menu;
		CTweakMenu_AddFloat((struct CTweakMenu *)menu, "X", &shot->toLoc[0], 10.0f, -10000.0f,
			10000.0f);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->toMenu, "Y", &shot->toLoc[1], 10.0f,
			-10000.0f, 10000.0f);
		CTweakMenu_AddFloat((struct CTweakMenu *)shot->toMenu, "Z", &shot->toLoc[2], 10.0f,
			-10000.0f, 10000.0f);
	}
	film = (struct TuShot *)TU_NEW(0x14c, TU_FILE_CAMERA, 0xfd);
	camera->filmCam = film;
	sprintf(film->name, "filmCam");
	camera->filmCam->endTime = 3600.0f;
	sprintf(camera->filmCam->entName, "player1");
	camera->filmCam->flags = 0;
	film = camera->filmCam;
	film->fromLoc[0] = 0.0f;
	film->fromLoc[1] = 1.0f;
	film->fromLoc[2] = 0.0f;
	film->fromLoc[3] = 1.0f;
	camera->filmCam->linkTime = 0.0f;
	sprintf(camera->filmCam->lomFile, "");
	film = camera->filmCam;
	film->from[0] = 0.0f;
	film->from[1] = 1.0f;
	film->from[2] = 0.0f;
	film->from[3] = 1.0f;
	film->to[0] = 0.0f;
	film->to[1] = 0.0f;
	film->to[2] = 0.0f;
	film->to[3] = 1.0f;
	film->menu = NULL;
	film->fromMenu = NULL;
	film->toMenu = NULL;
	film->next = NULL;
	film->orbitRadius = 1.0f;
	film->orbitTime = 1.0f;
	film->time = 5.0f;
	film->toLoc[0] = 0.0f;
	film->toLoc[1] = 0.0f;
	film->toLoc[2] = 0.0f;
	film->toLoc[3] = 1.0f;
	film->toLerpSpeed = 0.01f;
	film->fromType = 1;
	film->toType = 2;
	film->zoomType = 0;
	film->zoom = 0.0f;
	film->eye[0] = 0.0f;
	film->eye[1] = 1.0f;
	film->eye[2] = 0.0f;
	film->eye[3] = 1.0f;
	TUB_Reader_Dtor(&reader);
	return 1;
}

/* 0x001c70d0  TankedUp/CTank_Camera.cpp (inferred)
Frees the debug controller, a running LOM path, every shot with its tweak menus, the
debug root menu, and the filmCam. */
void CTankCamera_Free(void *self)
{
	struct TuCamera *camera = (struct TuCamera *)self;
	void *menu;

	if (camera->controller != NULL)
		TU_VFN(camera->controller, 0, void (*)(void *, int))(camera->controller, 1);
	if (camera->shot != NULL && camera->shot->lomFile[0] != 0)
		CLIntroCam_Destroy();
	while (camera->shots != NULL) {
		struct TuShot *shot;

		menu = camera->shots->fromMenu;
		if (menu != NULL) {
			CTweakMenu_Dtor((struct CTweakMenu *)menu);
			TU_DELETE(menu);
		}
		menu = camera->shots->toMenu;
		if (menu != NULL) {
			CTweakMenu_Dtor((struct CTweakMenu *)menu);
			TU_DELETE(menu);
		}
		menu = camera->shots->menu;
		if (menu != NULL) {
			CTweakMenu_Dtor((struct CTweakMenu *)menu);
			TU_DELETE(menu);
		}
		shot = camera->shots;
		camera->shots = shot->next;
		TU_DELETE(shot);
	}
	menu = TU_DBG_MENU;
	if (menu != NULL) {
		CTweakMenu_Dtor((struct CTweakMenu *)menu);
		TU_DELETE(menu);
	}
	if (camera->filmCam != NULL)
		TU_DELETE(camera->filmCam);
	camera->filmCam = NULL;
	TU_DBG_MENU = NULL;
}

/* 0x001c71c0  TankedUp/CTank_Camera.cpp (inferred)
A game state change picks the state's shot; the countdown also starts the HUD bars, the
exit starts a fade. */
void CTankCamera_StateChange(void *self, int from, int to)
{
	struct TuCamera *camera = (struct TuCamera *)self;

	(void)from;
	camera->state = to;
	if (to == 1) {
		CTankCamera_SetShot(self, "INTO CAM");
		return;
	}
	if (to == 2) {
		CTankCamera_SetShot(self, "LINK TO START CAM");
		return;
	}
	if (to == 3) {
		CTankCamera_SetShot(self, "COUNTDOWN CAM");
		CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
		return;
	}
	if (to == 4) {
		CTankCamera_SetShot(self, "GAME CAM");
		return;
	}
	if (to == 5) {
		CTankCamera_SetShot(self, "EXIT CAM");
		camera->fading = 1;
		return;
	}
	if (to == 6) {
		CTankCamera_SetShot(self, "DONE CAM");
		return;
	}
	if (to == 7)
		CTankCamera_SetShot(self, "FADED CAM");
}
