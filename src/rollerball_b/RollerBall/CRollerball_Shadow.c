/*
RollerBall/CRollerball_Shadow.c  (module rollerball_b)

C:\BlitzParty\Game\Src\RollerBall\CRollerball_Shadow.cpp (heap tag of 0x001967f0): the
projected ball shadows. A shadow (0x24 bytes) owns a shadow projector (an engine_misc
BCamera, 0x160 bytes) aimed down at up to five casters (pointers to actor instance pointer
fields of a player); the shadow system (game + 0x29d4) keeps the list of shadows, renders
their maps and binds them as texture stages 0-3 for the scene. The first functions
(before 0x001967f0) have no tag: placed by address order.

Shadow:        +0x00 next, +0x04 projector, +0x08 caster count, +0x0c casters[5],
               +0x20 the player entity (null: always drawn; else when its +0x1d is set)
Shadow system: +0x00 first shadow, +0x04 4 saved texture transforms (D3DTS_TEXTURE0-3)
*/

#include "rollerball_b.h"

#define RBSH_NEXT 0x00
#define RBSH_PROJECTOR 0x04
#define RBSH_COUNT 0x08
#define RBSH_CASTERS 0x0c
#define RBSH_PLAYER 0x20

/* bdisplay: the current field of view */
extern float DAT_002be588, DAT_002be58c;

/* a shadow is used when it has no player or its player is shown (+0x1d) */
static int rbb_ShadowShown(void *shadow)
{
	char *player = RBB_P(shadow, RBSH_PLAYER);

	return player == NULL || RBB_C(player, 0x1d) != 0;
}

/* 0x00196710  RollerBall/CRollerball_Shadow.cpp (inferred)
Render the casters into the shadow map: the first opens the pass (3), the last closes
it (4). */
void CRollerballShadow_Render(void *self)
{
	int count = RBB_I(self, RBSH_COUNT);
	int i;

	for (i = 0; i < count; i++) {
		unsigned char **caster = (unsigned char **)RBB_P(self, RBSH_CASTERS + i * 4);
		unsigned char flags;
		if (i == 0)
			flags = 3;
		else if (i == count - 1)
			flags = 4;
		else
			flags = 0;
		bProjector_Render((struct BCamera *)RBB_P(self, RBSH_PROJECTOR), *caster, 1, flags);
		count = RBB_I(self, RBSH_COUNT);
	}
}

/* 0x00196760  RollerBall/CRollerball_Shadow.cpp (inferred)
Add a caster (at most 5); the projector follows the first one. */
void CRollerballShadow_AddCaster(void *self, unsigned char **caster)
{
	if (RBB_I(self, RBSH_COUNT) >= 5)
		return;
	if (RBB_I(self, RBSH_COUNT) == 0)
		bCamera_Attach((struct BCamera *)RBB_P(self, RBSH_PROJECTOR), *caster, NULL, 1);
	RBB_P(self, RBSH_CASTERS + RBB_I(self, RBSH_COUNT) * 4) = caster;
	RBB_I(self, RBSH_COUNT) += 1;
}

/* 0x001967a0  RollerBall/CRollerball_Shadow.cpp (inferred)
Shadow system: put a shadow first in the list. */
void CRollerballShadowSystem_Add(void *self, void *shadow)
{
	if (RBB_P(self, 0) == NULL) {
		RBB_P(shadow, RBSH_NEXT) = NULL;
		RBB_P(self, 0) = shadow;
		return;
	}
	RBB_P(shadow, RBSH_NEXT) = RBB_P(self, 0);
	RBB_P(self, 0) = shadow;
}

/* 0x001967c0  RollerBall/CRollerball_Shadow.cpp (inferred)
Shadow system: move the projectors with their casters. */
void CRollerballShadowSystem_Update(void *self)
{
	char *s;

	for (s = RBB_P(self, 0); s != NULL; s = RBB_P(s, RBSH_NEXT))
		if (rbb_ShadowShown(s))
			bCamera_Update((struct BCamera *)RBB_P(s, RBSH_PROJECTOR));
}

/* 0x001967f0  RollerBall/CRollerball_Shadow.cpp
Make the shadow's projector (`type`, a `width` x `height` map) with the angles, distance,
field of view and clip range of the variation (tables at 0x002b51f8-0x002b5280). */
int CRollerballShadow_Init(void *self, int type, int width, int height, void *player)
{
	void *mem = RBB_NEW(0x160, "C:\\BlitzParty\\Game\\Src\\RollerBall\\CRollerball_Shadow.cpp", 0x5f);
	struct BCamera *cam = NULL;
	int v;

	if (mem != NULL)
		cam = bProjector_Construct((struct BCamera *)mem, type, width, height, 0);
	RBB_P(self, RBSH_PROJECTOR) = cam;
	if (cam == NULL)
		return 0;
	v = rbb_Variation;
	bCamera_SetAngles(cam, RBD_F(0x002b520c + v * 4), RBD_F(0x002b5220 + v * 4), 0.0f,
		RBD_F(0x002b51f8 + v * 4));
	bCamera_SetFov(cam, RBD_F(0x002b5234 + v * 4), RBD_F(0x002b5248 + v * 4),
		RBD_F(0x002b525c + v * 4), RBD_F(0x002b5270 + v * 4));
	bProjector_SetColour(cam, 0x80);
	bCamera_Update(cam);
	RBB_P(self, RBSH_PLAYER) = player;
	RBB_I(self, RBSH_COUNT) = 0;
	return 1;
}

/* 0x00196900  RollerBall/CRollerball_Shadow.cpp (inferred) */
int CRollerballShadowSystem_Init(void *self)
{
	RBB_P(self, 0) = NULL;
	return 1;
}

/* 0x00196910  RollerBall/CRollerball_Shadow.cpp (inferred)
Delete the shadows and their projectors. */
void CRollerballShadowSystem_Shutdown(void *self)
{
	char *s = RBB_P(self, 0);

	while (s != NULL) {
		char *next = RBB_P(s, RBSH_NEXT);
		void *cam = RBB_P(s, RBSH_PROJECTOR);
		if (cam != NULL)
			RBB_VFN(cam, 0x00, void *(*)(void *, int))(cam, 1);
		RBB_DELETE(s);
		s = next;
	}
}

/* 0x00196940  RollerBall/CRollerball_Shadow.cpp (inferred)
Render every shadow map, then bind them as texture stages (the texture transforms are
saved first, for 0x00196a00). */
void CRollerballShadowSystem_Begin(void *self)
{
	char *s;
	float fov_x;
	float fov_y;
	int stage;
	int i;

	for (i = 0; i < 4; i++)
		D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)(i + D3DTS_TEXTURE0),
			(D3DMATRIX *)RBB_ADDR(self, 4 + i * 0x40));
	fov_x = DAT_002be588;
	fov_y = DAT_002be58c;
	bdPushRenderState(4, 0, 0);
	for (s = RBB_P(self, 0); s != NULL; s = RBB_P(s, RBSH_NEXT))
		if (rbb_ShadowShown(s))
			CRollerballShadow_Render(s);
	bdSetFieldOfView(fov_x, fov_y);
	stage = 0;
	for (s = RBB_P(self, 0); s != NULL; s = RBB_P(s, RBSH_NEXT)) {
		if (rbb_ShadowShown(s)) {
			bProjector_Apply((struct BCamera *)RBB_P(s, RBSH_PROJECTOR), stage, 1);
			stage++;
		}
	}
	bdPopRenderState(4);
}

/* 0x00196a00  RollerBall/CRollerball_Shadow.cpp (inferred)
Unbind the shadow maps (projector virtual +0x08) and restore the texture transforms. */
void CRollerballShadowSystem_End(void *self)
{
	char *s;
	int stage = 0;
	int i;

	for (s = RBB_P(self, 0); s != NULL; s = RBB_P(s, RBSH_NEXT)) {
		if (rbb_ShadowShown(s)) {
			void *cam = RBB_P(s, RBSH_PROJECTOR);
			RBB_VFN(cam, 0x08, void (*)(void *, int))(cam, stage);
			stage++;
		}
	}
	D3DDevice_SetTextureState_TexCoordIndex(1, 1);
	for (i = 0; i < 4; i++)
		D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)(i + D3DTS_TEXTURE0),
			(const D3DMATRIX *)RBB_ADDR(self, 4 + i * 0x40));
}
