/*
ClassLib/unplaced_token.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\ClassLib\CToken.cpp (file inferred from the content; the function
   table places these four gap functions in Demolition)

Token helpers next to classlib_fx's CToken.c: load the token models and textures, draw the
glow sprites, walk the token list, update every token. The token list, models and textures
belong to classlib_fx (classlib_fx_data.c).
*/

#include "demolition_d.h"

/* classlib_fx: the token models ("bronzetoken.obe", "silvertoken.obe", ...: name, resource) */
struct DDTokenModel {
	char *name;
	void *actor;
};
extern struct DDTokenModel DAT_00298d30[3];
/* classlib_fx: shadow mode, glow texture, shadow texture, the token list (next, prev, count) */
extern int DAT_002ff9c8;
extern void *DAT_002ff9cc;
extern void *DAT_002ff9d0;
extern unsigned int DAT_002ff9d4[3];
/* the identity matrix (engine) */
extern float DAT_002855f0[16];

/* classlib_ui CLList_Next on the token list (ECX this) */
#define TOKEN_LIST_NEXT(node) ((char *)FUN_00071110(DAT_002ff9d4, (unsigned int *)(node)))

/* 0x0010c080  Demolition (gap; ClassLib/CToken.cpp by content)
Draws a glow sprite (tokenglow.bmp, additive, depth offset -1) on every token big enough. */
void CToken_DrawGlowSprites(void)
{
	char *tok;

	if (DAT_002ff9d4[2] == 0)
		return;
	FUN_00043930(DAT_002855f0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(7, 2, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00046cb0(5, 0, 0);
	FUN_00044550(-1.0f);
	for (tok = TOKEN_LIST_NEXT(NULL); tok != NULL; tok = TOKEN_LIST_NEXT(tok)) {
		if (!(DD_F32(tok, 0x1a0) < 0.00999999978f))
			FUN_00040750(DD_FP(tok, 0xd0), DD_F32(tok, 0x1a0) * 100.0f, DD_F32(tok, 0x1dc) * 100.0f,
			             DD_F32(tok, 0x1e4), (struct bdTexture *)DAT_002ff9cc, 0, 0, 0x7f, 0x7f, 0x7f,
			             0x30, NULL, 0.0f);
	}
	FUN_00044b00(10, 0, 0);
	FUN_00044b00(7, 3, 0);
	FUN_00046d00(5);
	FUN_00044550(0.0f);
}

/* 0x0010c190  Demolition (gap; ClassLib/CToken.cpp by content)
Loads the token models, the shadow texture (tokenshadow.bmp unless one is given) and the
glow texture, and sets the shadow mode. */
void CToken_LoadAssets(int shadowMode, void *shadowTexture)
{
	int i;

	for (i = 0; i < 3; i++)
		DAT_00298d30[i].actor = FUN_0004bb30(NULL, DAT_00298d30[i].name, 1, 0);
	if (shadowTexture == NULL)
		shadowTexture = FUN_0004bb30(NULL, "tokenshadow.bmp", 0, 0xdefa);
	DAT_002ff9d0 = shadowTexture;
	DAT_002ff9cc = FUN_0004bb30(NULL, "tokenglow.bmp", 0, 0xdefa);
	DAT_002ff9c8 = shadowMode;
}

/* 0x0010c200  Demolition (gap; ClassLib/CToken.cpp by content)
The token after `token` (the first for NULL), or NULL. */
void *CToken_Next(void *token)
{
	char *next;

	next = TOKEN_LIST_NEXT(token != NULL ? token : NULL);
	if (next == NULL)
		next = NULL;
	return next;
}

/* 0x0010ce90  Demolition (gap; ClassLib/CToken.cpp by content)
Updates every token (classlib_fx CToken_Update returns the node to go on from). */
void CToken_UpdateAll(void)
{
	char *tok;

	tok = TOKEN_LIST_NEXT(NULL);
	while (tok != NULL)
		tok = TOKEN_LIST_NEXT(FUN_0010cd60((struct CToken *)tok));
}
