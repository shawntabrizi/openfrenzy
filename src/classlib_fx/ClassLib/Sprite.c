/*
ClassLib/Sprite.c  (classlib_fx)
<- C:\BlitzParty\Game\Src\ClassLib\Sprite.cpp

Animated sprites: a definition is a set of numbered texture frames
("<name>00.bmp", "<name>01.bmp", ...), an animation plays one at a frame time, and a
sprite instance shows the current frame.
*/

#include "classlib_fx.h"

static const char s_SpriteFile[] = "C:\\BlitzParty\\Game\\Src\\ClassLib\\Sprite.cpp";

/* 0x00086580  ClassLib/Sprite.cpp
Finds the frames of `count` definitions in `package`: "<name>NN.bmp" (".tga" with flag
4) from 00 until one is missing. */
void Sprite_LoadDefs(CSpriteDef *defs, int count, int package)
{
	char name[60];
	int found[65];
	const char *format;
	int n;
	int i;

	for (; 0 < count; count--) {
		n = 0;
		do {
			i = n;
			if ((defs->flags & 4) == 0)
				format = "%s%02d.bmp";
			else
				format = "%s%02d.tga";
			sprintf(name, format, defs->name, i);
			found[i + 1] = CLF_FindTexture(package, name, 0, 0);
			if (found[i + 1] == 0) {
				if (i == 0)
					CLF_Printf("ERROR: TEXTURE '%s' not there!\n", name);
			} else {
				CLF_Printf("Loaded '%s' OK\n", name);
			}
			n = i + 1;
		} while (found[i + 1] != 0);
		defs->numFrames = (short)i;
		if (-1 < i - 1) {
			defs->frames = CLF_Alloc((int)(short)i << 2, s_SpriteFile, 0x3d, 0x2001);
			for (n = i - 1; -1 < n; n--)
				defs->frames[n] = found[n + 1];
		}
		defs = defs + 1;
	}
}

/* 0x00086680  ClassLib/Sprite.cpp
Frees the frame tables of `count` definitions. */
void Sprite_FreeDefs(CSpriteDef *defs, int count)
{
	for (; 0 < count; count--) {
		CLF_Free(defs->frames);
		defs->frames = NULL;
		defs = defs + 1;
	}
}

/* 0x000866b0  ClassLib/Sprite.cpp
Advances the animation by one tick (frame-rate scaled); loops or stops at the end. */
void Sprite_Animate(CSprite *self)
{
	CSpriteAnim *anim;
	int frame;

	anim = self->anim;
	if (0.0f < anim->frameTime && anim != NULL && (anim->flags & 1) == 0 &&
	    1 < anim->def->numFrames) {
		self->timer = DAT_0028fda8 + self->timer;
		if (anim->frameTime < self->timer) {
			do {
				frame = self->frame;
				self->frame = frame + 1;
				self->timer = self->timer - anim->frameTime;
				if ((int)anim->def->numFrames <= frame + 1) {
					if ((anim->flags & 4) == 0)
						self->frame = anim->def->numFrames + -1;
					else
						self->frame = 0;
				}
			} while (self->anim->frameTime < self->timer);
		}
		self->texture = anim->def->frames[self->frame];
	}
}

/* 0x00086750  ClassLib/Sprite.cpp
Animates and turns the sprite by spin * dt. */
void Sprite_Update(CSprite *self, float dt)
{
	Sprite_Animate(self);
	self->angle = dt * self->spin + self->angle;
}

/* 0x00086770  ClassLib/Sprite.cpp
Starts animation `anim` (at a random frame with flag 2). */
void Sprite_SetAnim(CSprite *self, CSpriteAnim *anim)
{
	self->anim = anim;
	if ((anim->flags & 2) == 0)
		self->frame = 0;
	else
		self->frame = (int)((longlong)CLF_Rand() % (longlong)(int)anim->def->numFrames);
	self->timer = 0.0f;
	if (0 < anim->def->numFrames) {
		self->texture = anim->def->frames[self->frame];
		return;
	}
	self->texture = 0;
}

/* 0x000867e0  ClassLib/Sprite.cpp
Constructor (allocates when `self` is NULL): a grey sprite at `pos`. */
CSprite *Sprite_Create(CSprite *self, float *pos, CSpriteAnim *anim, int user)
{
	if (self == NULL) {
		self = CLF_Alloc(sizeof(CSprite), s_SpriteFile, 0xa3, 0x2001);
		if (self == NULL)
			return NULL;
	}
	Sprite_SetAnim(self, anim);
	self->user = user;
	self->colour[0] = 0x7f;
	self->colour[1] = 0x7f;
	self->colour[2] = 0x7f;
	self->colour[3] = 0x7f;
	self->spin = 0.0f;
	self->angle = 0.0f;
	self->unknown23 = 0;
	self->unknown22 = 0;
	self->pos[0] = pos[0];
	self->pos[1] = pos[1];
	self->pos[2] = pos[2];
	self->pos[3] = pos[3];
	if (anim == NULL) {
		self->animFlags = 0;
		return self;
	}
	self->animFlags = (short)anim->flags;
	return self;
}
