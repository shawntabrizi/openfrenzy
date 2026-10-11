/*
BACTOR/xbShaderSelector.c  <-  C:\Babel\Xbox\Src\BACTOR\xbShaderSelector.cpp

The cache of generated vertex shaders (per lighting state, per shader key)
and the assembler call.

0x0003dd50 is tagged with xbShaderSelector.cpp; the functions after it are
placed here by address order (file inferred).
*/

#include <string.h>

#include "bactor.h"
#include "bactor_extern.h"

/* xbox_kernel.c */
void DbgBreakPoint(void);

/* see pcxbShaderSelector.c */
extern int bactor_maxLightingStates;
extern int bactor_numLightingStates;
extern int bactor_maxShaderModes;
extern struct BLightingState *bactor_lightingStates;
extern struct BLightingState *bactor_currentLighting;
extern char bactor_shaderText[0x1000];

/* 0x00258910: the vertex declaration of skinned actors (v0 position, v1
weights, v2 indices, v3 normal, v4 colour, v5 texture coordinates) */
static const DWORD s_declSkinned[8] = {
	0x20000000, 0x40320000, 0x40320001, 0x40450002, 0x40320003, 0x40400004, 0x40220005, 0xffffffff,
};
/* 0x00258930: the vertex declaration of rigid actors */
static const DWORD s_declRigid[6] = {
	0x20000000, 0x40320000, 0x40320003, 0x40400004, 0x40220005, 0xffffffff,
};

/* 0x0003dd50  BACTOR/xbShaderSelector.cpp */
void bInitShaderCache(void)
{
	struct BShaderMode *modes;
	int i;

	bactor_numLightingStates = 0;
	bactor_maxLightingStates = 0x40;
	bactor_maxShaderModes = 0x10;
	bactor_currentLighting = NULL;
	bactor_lightingStates = ba_bkAllocNamed(0x900, "C:\\Babel\\Xbox\\Src\\BACTOR\\xbShaderSelector.cpp", 0x44, 0x2001,
		"Vertex Shader Cache", 0);
	modes = ba_bkAllocNamed(bactor_maxShaderModes * bactor_maxLightingStates * (int)sizeof(struct BShaderMode),
		"C:\\Babel\\Xbox\\Src\\BACTOR\\xbShaderSelector.cpp", 0x46, 0x2001, "Vertex Shader Mode List", 0);
	bactor_lightingStates[0].modes = modes;
	for (i = 0; i < bactor_maxLightingStates; i++)
	{
		bactor_lightingStates[i].modes = bactor_lightingStates[0].modes + bactor_maxShaderModes * i;
		bactor_lightingStates[i].numModes = 0;
	}
}

/* 0x0003de20  BACTOR/xbShaderSelector.cpp (file inferred) */
void bShutdownShaderCache(void)
{
	int i;
	int j;

	if (bactor_lightingStates == NULL)
		return;
	for (i = 0; i < bactor_maxLightingStates; i++)
		for (j = 0; j < bactor_lightingStates[i].numModes; j++)
			D3DDevice_DeleteVertexShader(bactor_lightingStates[i].modes[j].handle);
	ba_bkFree(bactor_lightingStates[0].modes);
	bactor_lightingStates[0].modes = NULL;
	ba_bkFree(bactor_lightingStates);
	bactor_lightingStates = NULL;
}

/* 0x0003deb0  BACTOR/xbShaderSelector.cpp (file inferred)
deletes every generated shader */
void baFlushShaderCache(void)
{
	int i;
	int j;

	if (bactor_lightingStates == NULL)
		return;
	for (i = 0; i < bactor_maxLightingStates; i++)
	{
		for (j = 0; j < bactor_lightingStates[i].numModes; j++)
			D3DDevice_DeleteVertexShader(bactor_lightingStates[i].modes[j].handle);
		bactor_lightingStates[i].numModes = 0;
	}
	bactor_numLightingStates = 0;
	bactor_currentLighting = NULL;
}

/* 0x0003df30  BACTOR/xbShaderSelector.cpp (file inferred)
assembles the generated shader text and creates the vertex shader */
void bAssembleShader(struct BShaderMode *mode, struct BActorInstance *inst)
{
	XGBuffer *compiled = NULL;
	XGBuffer *errors = NULL;
	XGBuffer *constants = NULL;
	XGBuffer *listing = NULL;
	const DWORD *decl;
	HRESULT hr;

	ba_bkPrintf("bAssembleShader\n");
	hr = XGAssembleShader("baGenerateShader", bactor_shaderText, (UINT)strlen(bactor_shaderText), 0, &constants,
		&compiled, &errors, &listing, NULL, NULL, NULL);
	if (hr < 0)
	{
		ba_bkPrintf("baGenerateShader: Assembly failed: '%s':\n", ba_bdErrorString(hr));
		ba_bkPrintf(bactor_shaderText);
		ba_bkPrintf((const char *)XGBuffer_GetBufferPointer(errors));
		XGBuffer_Release(errors);
		if (DAT_002cc4f0 != 0)
			DbgBreakPoint();
		return;
	}
	ba_bkPrintf("D3DDevice_CreateVertexShader\n");
	if ((inst->actor->flags & 1) == 0)
		decl = s_declRigid;
	else
		decl = s_declSkinned;
	hr = D3DDevice_CreateVertexShader(decl, (const DWORD *)XGBuffer_GetBufferPointer(compiled), (unsigned long *)&mode->handle, 0);
	XGBuffer_Release(compiled);
	XGBuffer_Release(errors);
	XGBuffer_Release(constants);
	XGBuffer_Release(listing);
	if (hr < 0)
	{
		ba_bkPrintf("baGenerateShader: Vertex Creation FAILED: '%s':\n", ba_bdErrorString(hr));
		ba_bkPrintf(bactor_shaderText);
		return;
	}
	ba_bkPrintf("done\n");
}
