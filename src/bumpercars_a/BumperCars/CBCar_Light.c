/*
BumperCars/CBCar_Light.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Light.cpp   (module bumpercars_a)

The arena lights (CBCar_Game + 0x3604, vtable 0x002649e4): POINT, AMBIENT, SPOT,
DIRECTIONAL and ROAMINGSPOT blocks of the variation's LIGHTINFO file, each a BACTOR light
that may pulse between two colours, follow an entity, or sweep (roaming spots); plus the
winner spotlight. 0x00092dc0 is tagged with this file; the gap functions 0x000924e0-
0x00094240 are this class's methods.
*/

#include "bumpercars_a.h"
#include "d3d8_extra.h"
#include <strings.h>

static const char s_LightFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Light.cpp";

/* a light of the arena (0x17c bytes) */
struct BCarLight {
	int type;                   /* 0x000 0 point, 1 spot, 2 ambient, 3 directional, 4 roaming spot */
	int enabled;                /* 0x004 */
	int off;                    /* 0x008 */
	char name[0x100];           /* 0x00c */
	float pos[4];               /* 0x10c */
	float target[4];            /* 0x11c spot target, direction, or roaming angles (x, y) */
	int colour[3];              /* 0x12c COLOUR */
	int colour2[3];             /* 0x138 COLOUR2 (-1: no pulse) */
	int current[3];             /* 0x144 */
	float phase;                /* 0x150 */
	float pulseSpeed;           /* 0x154 PULSESPEED */
	float falloff;              /* 0x158 FALLOFF / ATTENUATION */
	float cone;                 /* 0x15c CONE (radians) */
	float speed[2];             /* 0x160 SPEED (roaming, radians) */
	float unknown168[2];        /* 0x168 */
	struct BLight *light;       /* 0x170 */
	unsigned char *follow;      /* 0x174 an entity */
	struct BCarLight *next;     /* 0x178 */
};
OF_ASSERT_OFFSET(struct BCarLight, light, 0x170);
OF_ASSERT_SIZE(struct BCarLight, 0x17c);

struct CBCar_Light {
	const void *const *vtable;  /* 0x00 0x002649e4 */
	struct BCarLight *first;    /* 0x04 */
	struct BLight *spot;        /* 0x08 the winner spotlight */
	struct BLight *sun;         /* 0x0c */
	int loaded;                 /* 0x10 */
};

/* bdisplay: colour component (0-255) to float (0x002bd220) */
extern float DAT_002bd220[];
/* bca_data.c: the winner spotlight's position */
extern float DAT_0029292c[4];

#define READ_LINE(r, buf) BCA_FN(FUN_001386c0, char *, (void *, char *))((r), (buf))
#define NEXT_LINE(r) BCA_FN(FUN_00138720, void, (void *))(r)
#define GET_STRING(text, dest) BCA_FN(FUN_000a4e00, int, (char *, char *, int))((text), (dest), 0x1ff)

/* 0x002649e4  CBCar_Light vtable */
const void *const vtable_002649e4[1] = {
	(const void *)FUN_000927d0, /* [0] scalar deleting destructor */
};

/* 0x000924e0  (gap): CBCar_Light::CBCar_Light */
void *CBCar_Light_Ctor(struct CBCar_Light *self)
{
	self->vtable = vtable_002649e4;
	self->first = NULL;
	return self;
}

/* 0x000924f0  (gap): CBCar_Light::~CBCar_Light */
void CBCar_Light_Dtor(struct CBCar_Light *self)
{
	self->vtable = vtable_002649e4;
}

/* 0x00092500  (gap): makes the light called `name` follow an entity. Returns the light. */
struct BCarLight *CBCar_Light_Follow(struct CBCar_Light *self, const char *name, unsigned char *entity)
{
	struct BCarLight *l;

	for (l = self->first; l != NULL; l = l->next) {
		if (l->enabled != 0 && l->off == 0 && strcasecmp(l->name, name) == 0) {
			l->follow = entity;
			return l;
		}
	}
	return NULL;
}

/* 0x00092560  (gap): switches the lights off (or on): the ambient light goes to grey */
void CBCar_Light_SetOff(struct CBCar_Light *self, int off)
{
	struct BCarLight *l;
	int r = 0x1e, g = 0x1e, b = 0x1e;

	for (l = self->first; l != NULL; l = l->next) {
		if (l->type == 2) {
			b = l->current[2];
			r = l->current[0];
			g = l->current[1];
		} else if (l->light != NULL) {
			baEnableLight(l->light, off == 0);
		}
		l->off = off;
	}
	if (self->sun != NULL)
		baEnableLight(self->sun, off);
	if (off != 0) {
		baSetAmbientLight(0x60, 0x60, 0x60);
		return;
	}
	baSetAmbientLight(r, g, b);
}

/* 0x00092610  (gap): sets the ambient light (from an AMBIENT block, else 0x1e grey) */
void CBCar_Light_ApplyAmbient(struct CBCar_Light *self)
{
	struct BCarLight *l = self->first;
	int found = 0;

	if (l != NULL) {
		do {
			if (l->enabled != 0 && l->off == 0 && l->type == 2) {
				found = 1;
				baSetAmbientLight(l->current[0], l->current[1], l->current[2]);
			}
			l = l->next;
		} while (l != NULL);
		if (found)
			return;
	}
	baSetAmbientLight(0x1e, 0x1e, 0x1e);
}

/* 0x00092680  (gap): draws the spot lights' cones (when lit and view != 0) and sets the
ambient light */
void CBCar_Light_Draw(struct CBCar_Light *self, byte view)
{
	struct BCarLight *l = self->first;
	int noLighting = CBCar_Debug_NoLighting((struct CBCar_Debug *)g_BCarGame->debug);
	int found = 0;

	if (l != NULL) {
		do {
			if (l->enabled != 0 && l->off == 0) {
				if (l->type == 1) {
					if ((view & (noLighting == 0)) != 0)
						BCA_FN(FUN_000a58e0, void, (float *, float *, float, int, int, int, int))(l->pos, l->target,
							l->cone, l->current[0], l->current[1], l->current[2], 0);
				} else if (l->type == 2) {
					found = 1;
					baSetAmbientLight(l->current[0], l->current[1], l->current[2]);
				}
			}
			l = l->next;
		} while (l != NULL);
		if (found)
			return;
	}
	baSetAmbientLight(0x1e, 0x1e, 0x1e);
}

/* 0x00092740  (gap): the winner spotlight, aimed at target */
void CBCar_Light_CreateSpot(struct CBCar_Light *self, float *target)
{
	self->spot = baCreateLight(NULL, 0);
	if (self->spot != NULL) {
		baSetSpotLight(self->spot, DAT_0029292c, target, 0x1.a36e2ep-15f, 0x1.657184p-3f);
		baSetLightColour(self->spot, 0x1e, 0x14, 0x14);
		baEnableLight(self->spot, 1);
	}
}

/* 0x000927a0  (gap): aims the winner spotlight */
void CBCar_Light_AimSpot(struct CBCar_Light *self, float *target)
{
	if (self->spot != NULL)
		baSetSpotLight(self->spot, DAT_0029292c, target, 0x1.a36e2ep-15f, 0x1.657184p-3f);
}

/* 0x000927d0  (gap): CBCar_Light scalar deleting destructor */
struct CBCar_Light *CBCar_Light_Destroy(struct CBCar_Light *self, byte flags)
{
	self->vtable = vtable_002649e4;
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

/* 0x000927f0  (gap): CBCar_Light::Shutdown */
void CBCar_Light_Shutdown(struct CBCar_Light *self)
{
	struct BCarLight *l;

	while (self->first != NULL) {
		l = self->first;
		self->first = l->next;
		if (l->light != NULL)
			baDeleteLight(l->light);
		BCA_DELETE(l);
	}
	if (self->spot != NULL) {
		baDeleteLight(self->spot);
		self->spot = NULL;
	}
	if (self->sun != NULL) {
		baDeleteLight(self->sun);
		self->sun = NULL;
	}
}

/* 0x00092860  (gap): gives a light's BACTOR light to Direct3D (the fixed-function pipeline):
attenuation sqrt(falloff) (or falloff squared-free when exact != 0) */
void CBCar_Light_SetD3DLight(struct BCarLight *l, int exact)
{
	unsigned char *light = (unsigned char *)l->light;
	D3DLIGHT8 d;

	if (light == NULL)
		return;
	memset(&d, 0, sizeof d);
	d.Diffuse.r = DAT_002bd220[BCA_I32(light, 0x14)];
	d.Diffuse.g = DAT_002bd220[BCA_I32(light, 0x18)];
	d.Diffuse.b = DAT_002bd220[BCA_I32(light, 0x1c)];
	d.Diffuse.a = 1.0f;
	d.Specular.r = DAT_002bd220[BCA_I32(light, 0x14)];
	d.Specular.g = DAT_002bd220[BCA_I32(light, 0x18)];
	d.Specular.b = DAT_002bd220[BCA_I32(light, 0x1c)];
	d.Specular.a = 1.0f;
	switch (BCA_I32(light, 0)) {
	case 1:
		d.Type = D3DLIGHT_DIRECTIONAL;
		d.Direction.x = -BCA_F32(light, 0x30);
		d.Direction.y = -BCA_F32(light, 0x34);
		d.Direction.z = -BCA_F32(light, 0x38);
		break;
	case 2:
		d.Type = D3DLIGHT_POINT;
		d.Position.x = BCA_F32(light, 0x30);
		d.Position.y = BCA_F32(light, 0x34);
		d.Position.z = BCA_F32(light, 0x38);
		d.Range = 1000000.0f;
		d.Falloff = 1.0f;
		if (exact == 0)
			d.Attenuation1 = bmFastSqrt(l->falloff);
		else
			d.Attenuation2 = l->falloff;
		break;
	case 3:
		d.Type = D3DLIGHT_SPOT;
		d.Position.x = BCA_F32(light, 0x30);
		d.Position.y = BCA_F32(light, 0x34);
		d.Position.z = BCA_F32(light, 0x38);
		d.Direction.x = BCA_F32(light, 0x40);
		d.Direction.y = BCA_F32(light, 0x44);
		d.Direction.z = BCA_F32(light, 0x48);
		d.Range = 1000000.0f;
		d.Falloff = 1.0f;
		if (exact == 0)
			d.Attenuation1 = bmFastSqrt(l->falloff);
		else
			d.Attenuation2 = l->falloff;
		d.Phi = BCA_F32(light, 0x64);
		break;
	default:
		return;
	}
	D3DDevice_SetLight((DWORD)BCA_I32(light, 0x20), &d);
}

/* 0x00092a10  (gap): sets a light's colour */
void CBCar_Light_SetColour(struct BCarLight *l, int r, int g, int b)
{
	if (l == NULL)
		return;
	l->colour[0] = r;
	l->colour[1] = g;
	l->colour[2] = b;
	if (l->light != NULL) {
		baSetLightColour(l->light, r, g, b);
		CBCar_Light_SetD3DLight(l, 0);
	}
}

/* 0x00092a70  (gap): pulses the colours, moves the following lights with their entity and
sweeps the roaming spots (the function runs on to 0x00092da0: Ghidra's 0x00092d60 is its
tail, not a function) */
void CBCar_Light_Update(struct CBCar_Light *self, int playing, float dt)
{
	struct BCarLight *l;
	unsigned char *e;
	double f, s;
	float dir[4], target[4];

	(void)playing;
	for (l = self->first; l != NULL; l = l->next) {
		if (l->enabled == 0 || l->off != 0)
			continue;
		f = (double)l->pulseSpeed * (double)0x1.45f306p-2f; /* 1/pi */
		if (l->colour2[0] == -1) {
			l->current[0] = l->colour[0];
			l->current[1] = l->colour[1];
			l->current[2] = l->colour[2];
		} else {
			l->phase = (float)((double)dt + (double)l->phase);
			s = fabs(sin((double)l->phase * f));
			l->current[0] = BCA_FTOL((double)(l->colour2[0] - l->colour[0]) * s + (double)l->colour[0]);
			l->current[1] = BCA_FTOL((double)(l->colour2[1] - l->colour[1]) * s + (double)l->colour[1]);
			l->current[2] = BCA_FTOL((double)(l->colour2[2] - l->colour[2]) * s + (double)l->colour[2]);
		}
		if (l->light != NULL) {
			baSetLightColour(l->light, l->current[0], l->current[1], l->current[2]);
			CBCar_Light_SetD3DLight(l, 0);
		}
		switch (l->type) {
		case 0:
			e = l->follow;
			if (e != NULL) {
				memcpy(l->target, e + 0x50, 16);
				l->target[3] = 1.0f;
				l->target[1] = BCA_F32(l->follow, 0xf4) + l->target[1];
				baSetPointLight(l->light, l->target, l->falloff);
			}
			break;
		case 1:
			e = l->follow;
			if (e != NULL) {
				memcpy(l->target, e + 0x50, 16);
				l->target[3] = 1.0f;
				l->target[1] = BCA_F32(l->follow, 0xf4) + l->target[1];
				baSetSpotLight(l->light, l->pos, l->target, l->falloff, l->cone);
			}
			break;
		case 4:
			e = l->follow;
			if (e != NULL) {
				memcpy(l->pos, e + 0x50, 16);
				l->pos[3] = 1.0f;
				l->pos[1] = BCA_F32(l->follow, 0xf4) + l->pos[1];
			}
			dir[0] = 0.0f;
			dir[1] = 0.0f;
			dir[2] = 7000.0f;
			FUN_00051220(target, dir, l->target[0], l->target[1]);
			target[0] = target[0] + l->pos[0];
			target[1] = target[1] + l->pos[1];
			target[2] = target[2] + l->pos[2];
			target[3] = 1.0f;
			baSetSpotLight(l->light, l->pos, target, l->falloff, l->cone);
			l->target[0] = l->speed[0] + l->target[0];
			l->target[1] = l->speed[1] + l->target[1];
			break;
		}
	}
}

static struct BCarLight *new_light(int line, int type, int withLight)
{
	struct BCarLight *l = (struct BCarLight *)BCA_NEW(0x17c, s_LightFile, line, 0x1001);

	if (l == NULL)
		return NULL;
	l->light = NULL;
	if (withLight) {
		l->light = baCreateLight(NULL, 0);
		if (l->light == NULL) {
			BCA_DELETE(l);
			return (struct BCarLight *)(uintptr_t)1; /* stop loading */
		}
	}
	l->type = type;
	l->follow = NULL;
	l->name[0] = 0;
	l->enabled = 1;
	l->off = 0;
	l->colour[2] = 0;
	l->colour[1] = 0;
	l->colour[0] = 0;
	l->colour2[2] = -1;
	l->colour2[1] = -1;
	l->colour2[0] = -1;
	l->pulseSpeed = 1.0f;
	l->pos[3] = 1.0f;
	l->pos[0] = 0.0f;
	l->pos[1] = 0.0f;
	l->pos[2] = 0.0f;
	l->target[3] = 1.0f;
	l->target[0] = 0.0f;
	l->target[1] = 0.0f;
	l->target[2] = 0.0f;
	return l;
}

/* the commands every block knows; returns the status (0 unknown, 1 done, 2 bad) */
static int common_command(struct BCarLight *l, char *text, int status, int name, int colours)
{
	char word[512];

	if (name && strncasecmp(text, "NAME ", 5) == 0) {
		status = 2;
		if (GET_STRING(text, word) != 0) {
			snprintf(l->name, 0xff, "%s", word);
			status = 1;
		}
	}
	(void)colours;
	return status;
}

static int colour_commands(struct BCarLight *l, char *text, int status)
{
	if (strncasecmp(text, "COLOUR ", 7) == 0) {
		status = 2;
		if (sscanf(text, "%*s%d%d%d", &l->colour[0], &l->colour[1], &l->colour[2]) == 3)
			status = 1;
	}
	if (strncasecmp(text, "COLOUR2 ", 8) == 0) {
		status = 2;
		if (sscanf(text, "%*s%d%d%d", &l->colour2[0], &l->colour2[1], &l->colour2[2]) == 3)
			status = 1;
	}
	return status;
}

static void report(struct BCarTextReader *reader, char *text, int status, int lineInvalid, int lineBad)
{
	char word[512];

	if (strcasecmp(text, "}") == 0)
		return;
	sscanf(text, "%s", word);
	if (status == 0)
		BCA_Printf("%s(%d) : LoadLights(\"%s\", %d): Invalid Command \"%s\"\n", s_LightFile, lineInvalid, reader->name,
			reader->line, word);
	else if (status == 2)
		BCA_Printf("%s(%d) : LoadLights(\"%s\", %d): Bad use of Command \"%s\" (%s)\n", s_LightFile, lineBad,
			reader->name, reader->line, word, text);
}

/* reads the next line of a block; returns NULL at its end ("}" or the end of the file) */
static char *block_line(struct BCarTextReader *reader, char *line, char **text)
{
	while (reader->cursor < reader->end) {
		if (strcasecmp(*text, "}") == 0)
			return NULL;
		*text = READ_LINE(reader, line);
		NEXT_LINE(reader);
		if (strncmp(*text, "//", 2) != 0)
			return *text;
	}
	return NULL;
}

/* 0x00092dc0  BumperCars/CBCar_Light.cpp: CBCar_Light::LoadLights */
void CBCar_Light_LoadLights(struct CBCar_Light *self, struct BCarTextReader *reader)
{
	char line[512];
	char *text;
	struct BCarLight *l;
	int status;
	float cone, a, b, target[4], dir[4];

	if (reader->end <= reader->cursor)
		return;
	do {
		text = READ_LINE(reader, line);
		NEXT_LINE(reader);
		if (strncmp(text, "//", 2) == 0)
			goto next;
		if (strcasecmp(text, "POINT {") == 0) {
			l = new_light(0x68, 0, 1);
			if (l == NULL || l == (struct BCarLight *)(uintptr_t)1)
				return;
			while (block_line(reader, line, &text) != NULL) {
				status = common_command(l, text, 0, 1, 0);
				if (strncasecmp(text, "TARGET ", 7) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f%f", &l->target[0], &l->target[1], &l->target[2]) == 3)
						status = 1;
				}
				status = colour_commands(l, text, status);
				if (strncasecmp(text, "FALLOFF ", 8) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->falloff) == 1)
						status = 1;
				}
				if (strncasecmp(text, "PULSESPEED ", 0xb) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->pulseSpeed) == 1)
						status = 1;
				}
				report(reader, text, status, 0xc0, 0xc2);
			}
			baSetPointLight(l->light, l->target, l->falloff);
			baSetLightColour(l->light, l->colour[0], l->colour[1], l->colour[2]);
			baEnableLight(l->light, 1);
			CBCar_Light_SetD3DLight(l, 0);
			l->next = self->first;
			self->first = l;
		}
		if (strcasecmp(text, "AMBIENT {") == 0) {
			l = new_light(0xd6, 2, 0);
			if (l == NULL)
				return;
			while (block_line(reader, line, &text) != NULL) {
				status = colour_commands(l, text, 0);
				if (strncasecmp(text, "PULSESPEED ", 0xb) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->pulseSpeed) == 1)
						status = 1;
				}
				report(reader, text, status, 0x10f, 0x111);
			}
			baSetAmbientLight(l->colour[0], l->colour[1], l->colour[2]);
			l->next = self->first;
			self->first = l;
		}
		if (strcasecmp(text, "SPOT {") == 0) {
			l = new_light(0x122, 1, 1);
			if (l == NULL || l == (struct BCarLight *)(uintptr_t)1)
				return;
			l->cone = 0.0f;
			l->falloff = 1.0f;
			while (block_line(reader, line, &text) != NULL) {
				status = common_command(l, text, 0, 1, 0);
				if (strncasecmp(text, "TARGET ", 7) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f%f", &l->target[0], &l->target[1], &l->target[2]) == 3)
						status = 1;
				}
				if (strncasecmp(text, "POSITION ", 9) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f%f", &l->pos[0], &l->pos[1], &l->pos[2]) == 3)
						status = 1;
				}
				status = colour_commands(l, text, status);
				if (strncasecmp(text, "CONE ", 5) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &cone) == 1) {
						l->cone = cone * 0x1.1df46ap-6f;
						status = 1;
					}
				}
				if (strncasecmp(text, "ATTENUATION ", 0xc) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->falloff) == 1)
						status = 1;
				}
				if (strncasecmp(text, "PULSESPEED ", 0xb) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->pulseSpeed) == 1)
						status = 1;
				}
				report(reader, text, status, 0x194, 0x196);
			}
			baSetSpotLight(l->light, l->pos, l->target, l->falloff, l->cone);
			baSetLightColour(l->light, l->colour[0], l->colour[1], l->colour[2]);
			baEnableLight(l->light, 1);
			CBCar_Light_SetD3DLight(l, 0);
			l->next = self->first;
			self->first = l;
		}
		if (strcasecmp(text, "DIRECTIONAL {") == 0) {
			l = new_light(0x1ab, 3, 1);
			if (l == NULL || l == (struct BCarLight *)(uintptr_t)1)
				return;
			while (block_line(reader, line, &text) != NULL) {
				status = common_command(l, text, 0, 1, 0);
				if (strncasecmp(text, "VECTOR ", 7) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f%f", &l->target[0], &l->target[1], &l->target[2]) == 3)
						status = 1;
				}
				status = colour_commands(l, text, status);
				if (strncasecmp(text, "PULSESPEED ", 0xb) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->pulseSpeed) == 1)
						status = 1;
				}
				report(reader, text, status, 0x1fc, 0x1fe);
			}
			baSetDirectionalLight(l->light, l->target);
			baSetLightColour(l->light, l->colour[0], l->colour[1], l->colour[2]);
			baEnableLight(l->light, 1);
			CBCar_Light_SetD3DLight(l, 0);
			l->next = self->first;
			self->first = l;
		}
		if (strcasecmp(text, "ROAMINGSPOT {") == 0) {
			l = new_light(0x212, 4, 1);
			if (l == NULL || l == (struct BCarLight *)(uintptr_t)1)
				return;
			l->cone = 0.0f;
			l->falloff = 1.0f;
			l->unknown168[1] = 1.0f;
			l->speed[0] = 0.0f;
			l->speed[1] = 0.0f;
			l->unknown168[0] = 0.0f;
			while (block_line(reader, line, &text) != NULL) {
				status = common_command(l, text, 0, 1, 0);
				if (strncasecmp(text, "ANGLE ", 6) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f", &a, &b) == 2) {
						status = 1;
						l->target[0] = a * 0x1.1df46ap-6f;
						l->target[1] = b * 0x1.1df46ap-6f;
					}
				}
				if (strncasecmp(text, "SPEED ", 6) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f", &a, &b) == 2) {
						status = 1;
						l->speed[0] = a * 0x1.1df46ap-6f;
						l->speed[1] = b * 0x1.1df46ap-6f;
					}
				}
				if (strncasecmp(text, "POSITION ", 9) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f%f%f", &l->pos[0], &l->pos[1], &l->pos[2]) == 3)
						status = 1;
				}
				status = colour_commands(l, text, status);
				if (strncasecmp(text, "CONE ", 5) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &cone) == 1) {
						l->cone = cone * 0x1.1df46ap-6f;
						status = 1;
					}
				}
				if (strncasecmp(text, "ATTENUATION ", 0xc) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->falloff) == 1)
						status = 1;
				}
				if (strncasecmp(text, "PULSESPEED ", 0xb) == 0) {
					status = 2;
					if (sscanf(text, "%*s%f", &l->pulseSpeed) == 1)
						status = 1;
				}
				report(reader, text, status, 0x299, 0x29b);
			}
			dir[0] = 0.0f;
			dir[1] = 0.0f;
			dir[2] = 7000.0f;
			FUN_00051220(target, dir, l->target[0], l->target[1]);
			target[0] = target[0] + l->pos[0];
			target[1] = target[1] + l->pos[1];
			target[2] = target[2] + l->pos[2];
			baSetSpotLight(l->light, l->pos, target, l->falloff, l->cone);
			baSetLightColour(l->light, l->colour[0], l->colour[1], l->colour[2]);
			baEnableLight(l->light, 1);
			CBCar_Light_SetD3DLight(l, 0);
			l->next = self->first;
			self->first = l;
		}
next:
		;
	} while (reader->cursor < reader->end);
}

/* 0x00094240  (gap): CBCar_Light::Initialise: loads the light file and adds a grey sun.
Returns 1 when loaded. */
int CBCar_Light_Initialise(struct CBCar_Light *self, struct bkPackage *package, char *file)
{
	struct BCarTextReader reader;
	uint size;
	void *text;
	float dir[4];

	BCA_FN(FUN_000a5be0, void, (void *))(&reader);
	text = bkLoadFile(package, file, NULL, &size, NULL, 0);
	if (text == NULL) {
		BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
		return 0;
	}
	BCA_FN(FUN_000a5c00, void, (void *, void *, uint, const char *))(&reader, text, size, file);
	CBCar_Light_LoadLights(self, &reader);
	self->loaded = 1;
	self->sun = baCreateLight(NULL, 0);
	if (self->sun != NULL) {
		dir[0] = 0.0f;
		dir[1] = 0x1.69fbe8p-1f;   /* 0.7071 */
		dir[2] = -0x1.69fbe8p-1f;
		dir[3] = 1.0f;
		baSetDirectionalLight(self->sun, dir);
		baSetLightColour(self->sun, 0x7f, 0x7f, 0x7f);
	}
	BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
	return 1;
}
