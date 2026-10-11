/*
D3D8_FF.H

The Direct3D 8 fixed-function pipeline on top of the NV2A translators
(d3d8_ff.c), as the Xbox's Direct3D runtime does it:

- a vertex shader handle that is an FVF code (even: programmable handles
  are odd) is a fixed-function vertex: its elements come from the code, and
  its GLSL from the transform, lighting, texture coordinate and fog state
  (d3d8_ff_vertex_key, d3d8_ff_vertex_glsl);
- without a pixel shader program, the texture stage states are turned into
  register combiner state (d3d8_ff_combiners), which nv2a_psh.c translates
  like any other.

Internal to src/d3d8 (d3d8_gl.c uses it; the tests in src/d3d8/tests too).
Game code uses d3d8_extra.h.
*/

#ifndef OPENFRENZY_D3D8_FF_H
#define OPENFRENZY_D3D8_FF_H

#include "xgpu.h"
#include "d3d8_extra.h"

/* ---------- vertex layout of an FVF code */

struct d3d8_ff_element
{
	unsigned char reg;   /* D3DVSDE_* */
	unsigned char type;  /* D3DVSDT_* */
	unsigned char bytes;
	unsigned short offset;
};

/* the elements (stream 0) of an FVF code, in vertex order; returns their
count (at most `maximum`) and sets the vertex size */
unsigned long d3d8_ff_fvf_elements(DWORD fvf, struct d3d8_ff_element *elements, unsigned long maximum,
	unsigned long *stride);

/* ---------- vertex processing */

#define D3D8_FF_MAX_LIGHTS 8

/* everything the generated vertex shader depends on (compared and hashed
whole: build it with d3d8_ff_vertex_key_build, which zeroes it first) */
struct d3d8_ff_vertex_key
{
	DWORD fvf;
	unsigned char lighting;
	unsigned char specular;
	unsigned char local_viewer;
	unsigned char normalize;
	/* D3DMCS_* for diffuse, ambient, specular, emissive (COLORVERTEX off:
	all D3DMCS_MATERIAL) */
	unsigned char material_source[4];
	unsigned char light_count;
	unsigned char light_type[D3D8_FF_MAX_LIGHTS];
	/* 0 no fog coordinate needed, 1 eye depth, 2 eye distance (range
	fog), 3 the specular alpha (vertex fog) */
	unsigned char fog;
	/* per stage: the coordinate set, its generation (D3DTSS_TCI_* >> 16)
	and the transform (count in bits 0-2, projected in bit 3) */
	unsigned char texture_index[4];
	unsigned char texture_generation[4];
	unsigned char texture_transform[4];
	/* D3DRS_POINTSCALEENABLE (and not pretransformed) */
	unsigned char point_scale;
	unsigned char reserved[2];
};

/* the key for the current render and texture stage states and lights */
void d3d8_ff_vertex_key_build(DWORD fvf, struct d3d8_ff_vertex_key *key);

/* GLSL for a fixed-function vertex; outputs as nv2a_vsh.c's (xD0, xT0, ...,
gl_Position as its clip-space path). Uniforms: ff[D3D8_FF_UNIFORM_COUNT],
viewport_scale, viewport_offset, point_scale, point_base, screen_offset. malloc'd. */
char *d3d8_ff_vertex_glsl(const struct d3d8_ff_vertex_key *key);

/* ff[]: the matrices, material, ambient light and lights, in camera space */
#define D3D8_FF_UNIFORM_COUNT (34 + D3D8_FF_MAX_LIGHTS * 7)
#define D3D8_FF_WORLDVIEW 0
#define D3D8_FF_PROJECTION 4
#define D3D8_FF_NORMAL 8
#define D3D8_FF_TEXTURE 12
#define D3D8_FF_MATERIAL 28
#define D3D8_FF_AMBIENT 33
#define D3D8_FF_LIGHTS 34

/* brings ff[] up to date with the transforms (D3DTS_MAX of them) and the
light and material state; returns TRUE if anything differs from the last
call. uniforms keeps the values between calls: only the groups whose
inputs changed (d3d8_ff_transform_changed, SetMaterial, SetLight,
LightEnable, D3DRS_AMBIENT) are computed and compared again (all of them
when the array is not the one of the last call) */
BOOL d3d8_ff_uniforms(const D3DMATRIX *transforms, float uniforms[D3D8_FF_UNIFORM_COUNT][4]);

/* D3DDevice_SetTransform calls this for each transform it sets */
void d3d8_ff_transform_changed(DWORD state);

/* debug.draw_cache_check: TRUE if uniforms equal ff[] computed whole from
the current state */
BOOL d3d8_ff_uniforms_check(const D3DMATRIX *transforms, const float uniforms[D3D8_FF_UNIFORM_COUNT][4]);

/* ---------- pixel processing */

/* what is bound to each stage: */
enum
{
	_d3d8_ff_texture_none = 0,
	_d3d8_ff_texture_2d,
	_d3d8_ff_texture_3d,
	_d3d8_ff_texture_cube,
};

/* the register combiner state (D3DRS_PSALPHAINPUTS0 up to D3DRS_PS_MAX),
and the texture modes (D3DRS_PSTEXTUREMODES), that the texture stage states
give: D3DTSS_COLOROP and the rest, D3DRS_TEXTUREFACTOR, D3DRS_FOGENABLE and
D3DRS_SPECULARENABLE */
void d3d8_ff_combiners(const unsigned char texture_kind[D3DTSS_MAXSTAGES], DWORD state[D3DRS_PS_MAX],
	DWORD *texture_modes);

#endif
