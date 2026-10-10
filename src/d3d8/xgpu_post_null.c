/*
XGPU_POST_NULL.C

openfrenzy d3d8 module: the FXAA/SMAA post-processing entry points of OpenCE's
xgpu_post.c (which needs zlib and the SMAA lookup textures). Neither is
available, so the renderer runs without anti-aliasing (display.anti_aliasing
is "off" in of_config.c). Replace with the real xgpu_post.c to enable it.
*/

#include "platform.h"
#include "xgpu.h"

BOOL xgpu_post_prepare(BOOL smaa)
{
	(void)smaa;
	return FALSE;
}

BOOL xgpu_post_anti_alias(BOOL smaa, GLuint framebuffer, unsigned long width, unsigned long height,
	const GLint corners[4])
{
	(void)smaa;
	(void)framebuffer;
	(void)width;
	(void)height;
	(void)corners;
	return FALSE;
}
