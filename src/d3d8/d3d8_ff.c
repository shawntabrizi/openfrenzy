/*
D3D8_FF.C

The Direct3D 8 fixed-function pipeline for the GLES device (d3d8_gl.c):
FVF vertex layouts, transform and lighting as generated GLSL, texture stage
states as NV2A register combiner state, and the material and light entry
points. See d3d8_ff.h.

New for openfrenzy (Fuzion Frenzy draws almost everything with FVF codes
and texture stage states; Halo, which the rest of this layer was written
for, uses programmable shaders only). Lighting follows the public Direct3D 8
lighting equations; the combiner encoding is the one nv2a_psh.c decodes.
*/

#include "d3d8_ff.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- FVF codes */

static unsigned long texture_coordinate_count(DWORD fvf, unsigned long set)
{
	switch ((fvf >> (16 + set * 2)) & 3)
	{
	case D3DFVF_TEXTUREFORMAT1: return 1;
	case D3DFVF_TEXTUREFORMAT3: return 3;
	case D3DFVF_TEXTUREFORMAT4: return 4;
	default: return 2;
	}
}

static unsigned char float_type(unsigned long count)
{
	switch (count)
	{
	case 1: return D3DVSDT_FLOAT1;
	case 2: return D3DVSDT_FLOAT2;
	case 3: return D3DVSDT_FLOAT3;
	default: return D3DVSDT_FLOAT4;
	}
}

unsigned long d3d8_ff_fvf_elements(DWORD fvf, struct d3d8_ff_element *elements, unsigned long maximum,
	unsigned long *stride)
{
	struct d3d8_ff_element list[16];
	unsigned long count = 0, offset = 0, set, sets, weights = 0, index;

#define ADD_ELEMENT(r, t, n) \
	do { list[count].reg = (r); list[count].type = (t); list[count].bytes = (unsigned char)(n); \
		list[count].offset = (unsigned short)offset; offset += (n); count++; } while (0)

	switch (fvf & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ: ADD_ELEMENT(D3DVSDE_POSITION, D3DVSDT_FLOAT3, 12); break;
	case D3DFVF_XYZRHW: ADD_ELEMENT(D3DVSDE_POSITION, D3DVSDT_FLOAT4, 16); break;
	case D3DFVF_XYZB1: weights = 1; break;
	case D3DFVF_XYZB2: weights = 2; break;
	case D3DFVF_XYZB3: weights = 3; break;
	case D3DFVF_XYZB4: weights = 4; break;
	default: break;
	}
	if (weights)
	{
		ADD_ELEMENT(D3DVSDE_POSITION, D3DVSDT_FLOAT3, 12);
		ADD_ELEMENT(D3DVSDE_BLENDWEIGHT, float_type(weights), weights * 4);
	}
	if (fvf & D3DFVF_NORMAL)
		ADD_ELEMENT(D3DVSDE_NORMAL, D3DVSDT_FLOAT3, 12);
	if (fvf & D3DFVF_PSIZE)
		ADD_ELEMENT(D3DVSDE_POINTSIZE, D3DVSDT_FLOAT1, 4);
	if (fvf & D3DFVF_DIFFUSE)
		ADD_ELEMENT(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR, 4);
	if (fvf & D3DFVF_SPECULAR)
		ADD_ELEMENT(D3DVSDE_SPECULAR, D3DVSDT_D3DCOLOR, 4);
	sets = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	if (sets > 4)
		sets = 4;
	for (set = 0; set < sets; set++)
	{
		unsigned long n = texture_coordinate_count(fvf, set);

		ADD_ELEMENT((unsigned char)(D3DVSDE_TEXCOORD0 + set), float_type(n), n * 4);
	}
#undef ADD_ELEMENT
	if (stride)
		*stride = offset;
	if (count > maximum)
		count = maximum;
	for (index = 0; index < count; index++)
		elements[index] = list[index];
	return count;
}

/* ---------- material and lights */

/* light indices the game may use (it numbers its lights from 0) */
#define LIGHT_SLOTS 64

static D3DMATERIAL8 material =
{
	{ 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f,
};

static struct
{
	D3DLIGHT8 light;
	BOOL set;
	BOOL enabled;
} lights[LIGHT_SLOTS];

/* 0x001f54a0 */
void WINAPI D3DDevice_SetMaterial(CONST D3DMATERIAL8 *new_material)
{
	if (new_material)
		material = *new_material;
}

/* 0x001f5cd0 */
HRESULT WINAPI D3DDevice_SetLight(DWORD index, CONST D3DLIGHT8 *light)
{
	if (!light)
		return D3DERR_INVALIDCALL;
	if (index >= LIGHT_SLOTS)
	{
		platform_log("Direct3D: light %lu is past the %d lights kept: ignored", (unsigned long)index, LIGHT_SLOTS);
		return E_OUTOFMEMORY;
	}
	lights[index].light = *light;
	lights[index].set = TRUE;
	return S_OK;
}

/* 0x001f5eb0 */
HRESULT WINAPI D3DDevice_LightEnable(DWORD index, BOOL enable)
{
	if (index >= LIGHT_SLOTS)
	{
		platform_log("Direct3D: light %lu is past the %d lights kept: ignored", (unsigned long)index, LIGHT_SLOTS);
		return E_OUTOFMEMORY;
	}
	if (!lights[index].set)
	{
		/* a light enabled before it was set is the default one: white,
		directional, along +z (as the library's LightEnable does) */
		memset(&lights[index].light, 0, sizeof(lights[index].light));
		lights[index].light.Type = D3DLIGHT_DIRECTIONAL;
		lights[index].light.Diffuse.r = lights[index].light.Diffuse.g = lights[index].light.Diffuse.b = 1.0f;
		lights[index].light.Direction.z = 1.0f;
		lights[index].set = TRUE;
	}
	lights[index].enabled = enable != 0;
	return S_OK;
}

/* the enabled lights the vertex shader uses (the first eight, by index) */
static unsigned long active_lights(unsigned long *indices)
{
	unsigned long index, count = 0;

	for (index = 0; index < LIGHT_SLOTS && count < D3D8_FF_MAX_LIGHTS; index++)
	{
		if (lights[index].enabled)
			indices[count++] = index;
	}
	return count;
}

/* ---------- vertex key */

void d3d8_ff_vertex_key_build(DWORD fvf, struct d3d8_ff_vertex_key *key)
{
	const DWORD *rs = D3D__RenderState;
	unsigned long indices[D3D8_FF_MAX_LIGHTS], count, index;
	BOOL transformed = (fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW;
	int stage;

	memset(key, 0, sizeof(*key));
	key->fvf = fvf;
	if (!transformed && rs[D3DRS_LIGHTING])
	{
		key->lighting = 1;
		key->specular = rs[D3DRS_SPECULARENABLE] != 0;
		key->local_viewer = rs[D3DRS_LOCALVIEWER] != 0;
		key->normalize = rs[D3DRS_NORMALIZENORMALS] != 0;
		if (rs[D3DRS_COLORVERTEX])
		{
			key->material_source[0] = (unsigned char)rs[D3DRS_DIFFUSEMATERIALSOURCE];
			key->material_source[1] = (unsigned char)rs[D3DRS_AMBIENTMATERIALSOURCE];
			key->material_source[2] = (unsigned char)rs[D3DRS_SPECULARMATERIALSOURCE];
			key->material_source[3] = (unsigned char)rs[D3DRS_EMISSIVEMATERIALSOURCE];
		}
		count = active_lights(indices);
		key->light_count = (unsigned char)count;
		for (index = 0; index < count; index++)
			key->light_type[index] = (unsigned char)lights[indices[index]].light.Type;
	}
	if (rs[D3DRS_FOGENABLE])
	{
		if (rs[D3DRS_FOGTABLEMODE] == D3DFOG_NONE)
			key->fog = 3;
		else
			key->fog = rs[D3DRS_RANGEFOGENABLE] && !transformed ? 2 : 1;
	}
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		DWORD coordinates = D3D__TextureState[stage][D3DTSS_TEXCOORDINDEX];
		DWORD flags = D3D__TextureState[stage][D3DTSS_TEXTURETRANSFORMFLAGS];

		key->texture_index[stage] = (unsigned char)(coordinates & 0xffff);
		key->texture_generation[stage] = transformed ? 0 : (unsigned char)((coordinates >> 16) & 0xff);
		key->texture_transform[stage] = (unsigned char)((flags & 7) | ((flags & D3DTTFF_PROJECTED) ? 8 : 0));
	}
}

/* ---------- vertex GLSL */

static const char *material_color(int source, BOOL has_diffuse, BOOL has_specular, int fallback)
{
	static char buffer[4][32];
	static int next;
	char *result = buffer[next++ & 3];

	if (source == D3DMCS_COLOR1 && has_diffuse)
		return "vD";
	if (source == D3DMCS_COLOR2 && has_specular)
		return "vS";
	snprintf(result, sizeof(buffer[0]), "ff[%d]", D3D8_FF_MATERIAL + fallback);
	return result;
}

char *d3d8_ff_vertex_glsl(const struct d3d8_ff_vertex_key *key)
{
	struct xgpu_text text = { 0 };
	struct d3d8_ff_element elements[16];
	unsigned long count, index, sets[4] = { 0 };
	BOOL present[XGPU_VERTEX_ATTRIBUTE_COUNT] = { FALSE };
	BOOL transformed = (key->fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW;
	BOOL has_normal, has_diffuse, has_specular;
	int stage, light;

	count = d3d8_ff_fvf_elements(key->fvf, elements, 16, NULL);
	for (index = 0; index < count; index++)
	{
		present[elements[index].reg] = TRUE;
		if (elements[index].reg >= D3DVSDE_TEXCOORD0 && elements[index].reg <= D3DVSDE_TEXCOORD3)
			sets[elements[index].reg - D3DVSDE_TEXCOORD0] = elements[index].bytes / 4;
	}
	has_normal = present[D3DVSDE_NORMAL];
	has_diffuse = present[D3DVSDE_DIFFUSE];
	has_specular = present[D3DVSDE_SPECULAR];

#ifdef HALO_GLES
	xgpu_text_append(&text, "#version %s\nprecision highp float;\nprecision highp int;\n",
		xgpu_capabilities.shading_language);
#else
	xgpu_text_append(&text, "#version 450 core\n");
#endif
	xgpu_text_append(&text,
		"/* fixed function, FVF 0x%lx */\n"
		"uniform vec4 ff[%d];\n"
		"uniform vec4 viewport_scale;\n"
		"uniform vec4 viewport_offset;\n"
		"uniform float point_size;\n"
		"uniform float screen_offset;\n"
		"out vec4 xD0;\nout vec4 xD1;\nout vec4 xB0;\nout vec4 xB1;\n"
		"out vec4 xT0;\nout vec4 xT1;\nout vec4 xT2;\nout vec4 xT3;\n"
		"out float xFog;\n"
		"invariant gl_Position;\n",
		(unsigned long)key->fvf, D3D8_FF_UNIFORM_COUNT);
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		if (present[index])
			xgpu_text_append(&text, "layout(location = %lu) in vec4 v%lu;\n", index, index);
	}
	/* a row vector times a matrix held as four rows */
	xgpu_text_append(&text,
		"#define XFORM(p, m) ((p).x * ff[(m)] + (p).y * ff[(m) + 1] + (p).z * ff[(m) + 2] + (p).w * ff[(m) + 3])\n"
		"void main()\n{\n"
		"\tvec4 vD = %s;\n"
		"\tvec4 vS = %s;\n"
		"\tvec4 position;\n"
		"\tvec3 P = vec3(0.0);\n"
		"\tvec3 N = vec3(0.0);\n",
		has_diffuse ? "v3" : "vec4(1.0)", has_specular ? "v4" : "vec4(0.0, 0.0, 0.0, 1.0)");

	if (transformed)
	{
		/* screen coordinates and 1/w: back to clip space, as the vertex
		program path does (nv2a_vsh.c). z is 0 to 1; a z past 1 is taken
		to be in depth buffer units, as the Xbox's hardware would read it */
		xgpu_text_append(&text,
			"\tvec3 scale = vec3(viewport_scale.x != 0.0 ? viewport_scale.x : 1.0,\n"
			"\t\tviewport_scale.y != 0.0 ? viewport_scale.y : 1.0, viewport_scale.z != 0.0 ? viewport_scale.z : 1.0);\n"
			"\tfloat w = v0.w != 0.0 ? 1.0 / v0.w : 1.0;\n"
			"\tvec2 ndc = (v0.xy + vec2(0.5 + screen_offset, 0.5) - viewport_offset.xy) / scale.xy;\n"
			"\tfloat z = v0.z > 1.0001 ? (v0.z - viewport_offset.z) / scale.z : v0.z;\n"
			"\tposition = vec4(ndc * w, z * w, w);\n"
			"\tP = vec3(0.0, 0.0, w);\n");
		xgpu_text_append(&text, "\tvec4 color0 = vD;\n\tvec4 color1 = vS;\n");
	}
	else
	{
		xgpu_text_append(&text,
			"\tvec4 eye = XFORM(vec4(v0.xyz, 1.0), %d);\n"
			"\tP = eye.xyz;\n"
			"\tvec4 clip = XFORM(eye, %d);\n"
			"\tvec2 scale = vec2(viewport_scale.x != 0.0 ? viewport_scale.x : 1.0,\n"
			"\t\tviewport_scale.y != 0.0 ? viewport_scale.y : 1.0);\n"
			"\tposition = vec4(clip.xy + vec2(0.5 + screen_offset, 0.5) * clip.w / scale, clip.z, clip.w);\n",
			D3D8_FF_WORLDVIEW, D3D8_FF_PROJECTION);
		if (has_normal)
		{
			xgpu_text_append(&text, "\tN = v2.x * ff[%d].xyz + v2.y * ff[%d].xyz + v2.z * ff[%d].xyz;\n",
				D3D8_FF_NORMAL, D3D8_FF_NORMAL + 1, D3D8_FF_NORMAL + 2);
			if (key->normalize)
				xgpu_text_append(&text, "\tN = N * inversesqrt(max(dot(N, N), 1.0e-24));\n");
		}
		if (!key->lighting)
		{
			xgpu_text_append(&text, "\tvec4 color0 = vD;\n\tvec4 color1 = vS;\n");
		}
		else
		{
			const char *diffuse = material_color(key->material_source[0], has_diffuse, has_specular, 0);
			const char *ambient = material_color(key->material_source[1], has_diffuse, has_specular, 1);
			const char *specular = material_color(key->material_source[2], has_diffuse, has_specular, 2);
			const char *emissive = material_color(key->material_source[3], has_diffuse, has_specular, 3);

			xgpu_text_append(&text,
				"\tvec4 Md = %s;\n\tvec4 Ma = %s;\n\tvec4 Ms = %s;\n\tvec4 Me = %s;\n"
				"\tvec3 ambient_sum = vec3(0.0), diffuse_sum = vec3(0.0), specular_sum = vec3(0.0);\n"
				"\tvec3 V = %s;\n",
				diffuse, ambient, specular, emissive,
				key->local_viewer ? "-P * inversesqrt(max(dot(P, P), 1.0e-24))" : "vec3(0.0, 0.0, -1.0)");
			for (light = 0; light < key->light_count; light++)
			{
				int base = D3D8_FF_LIGHTS + light * 7;

				xgpu_text_append(&text, "\t{\n\t\t/* light %d */\n\t\tvec3 L;\n\t\tfloat attenuation = 1.0;\n", light);
				if (key->light_type[light] == D3DLIGHT_DIRECTIONAL)
				{
					xgpu_text_append(&text, "\t\tL = -ff[%d].xyz;\n", base + 4);
				}
				else
				{
					xgpu_text_append(&text,
						"\t\tvec3 to_light = ff[%d].xyz - P;\n"
						"\t\tfloat distance = length(to_light);\n"
						"\t\tL = to_light / max(distance, 1.0e-12);\n"
						"\t\tattenuation = distance <= ff[%d].x ?\n"
						"\t\t\t1.0 / max(ff[%d].z + ff[%d].w * distance + ff[%d].x * distance * distance, 1.0e-12) : 0.0;\n",
						base + 3, base + 5, base + 5, base + 5, base + 6);
					if (key->light_type[light] == D3DLIGHT_SPOT)
					{
						xgpu_text_append(&text,
							"\t\tfloat rho = dot(-L, ff[%d].xyz);\n"
							"\t\tattenuation *= rho > ff[%d].y ? 1.0 : (rho <= ff[%d].z ? 0.0 :\n"
							"\t\t\tpow(max((rho - ff[%d].z) / max(ff[%d].y - ff[%d].z, 1.0e-6), 0.0), ff[%d].y));\n",
							base + 4, base + 6, base + 6, base + 6, base + 6, base + 6, base + 5);
					}
				}
				xgpu_text_append(&text,
					"\t\tfloat facing = dot(N, L);\n"
					"\t\tambient_sum += ff[%d].rgb * attenuation;\n"
					"\t\tif (facing > 0.0)\n\t\t{\n"
					"\t\t\tdiffuse_sum += ff[%d].rgb * facing * attenuation;\n",
					base + 2, base);
				if (key->specular)
				{
					xgpu_text_append(&text,
						"\t\t\tvec3 H = normalize(L + V);\n"
						"\t\t\tspecular_sum += ff[%d].rgb * pow(max(dot(N, H), 0.0), ff[%d].x) * attenuation;\n",
						base + 1, D3D8_FF_MATERIAL + 4);
				}
				xgpu_text_append(&text, "\t\t}\n\t}\n");
			}
			xgpu_text_append(&text,
				"\tvec4 color0 = vec4(Me.rgb + Ma.rgb * ff[%d].rgb + Ma.rgb * ambient_sum + Md.rgb * diffuse_sum, Md.a);\n"
				"\tvec4 color1 = vec4(%s, vS.a);\n",
				D3D8_FF_AMBIENT, key->specular ? "Ms.rgb * specular_sum" : "vec3(0.0)");
		}
	}

	/* texture coordinates */
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		unsigned long set = key->texture_index[stage];
		unsigned long transform_count = key->texture_transform[stage] & 7;
		BOOL projected = (key->texture_transform[stage] & 8) != 0;
		unsigned long size;

		xgpu_text_append(&text, "\t{\n\t\tvec4 t;\n");
		switch (key->texture_generation[stage])
		{
		case D3DTSS_TCI_CAMERASPACENORMAL >> 16:
			xgpu_text_append(&text, "\t\tt = vec4(N, 1.0);\n");
			size = 3;
			break;
		case D3DTSS_TCI_CAMERASPACEPOSITION >> 16:
			xgpu_text_append(&text, "\t\tt = vec4(P, 1.0);\n");
			size = 3;
			break;
		case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR >> 16:
			xgpu_text_append(&text, "\t\tt = vec4(reflect(P * inversesqrt(max(dot(P, P), 1.0e-24)), N), 1.0);\n");
			size = 3;
			break;
		default:
			size = set < 4 ? sets[set] : 0;
			/* missing components as Direct3D fills them for the texture
			transform: (u) is (u, 1, 0, 0), (u, v) is (u, v, 1, 0) */
			switch (size)
			{
			case 1: xgpu_text_append(&text, "\t\tt = vec4(v%lu.x, 1.0, 0.0, 0.0);\n", D3DVSDE_TEXCOORD0 + set); break;
			case 2: xgpu_text_append(&text, "\t\tt = vec4(v%lu.xy, 1.0, 0.0);\n", D3DVSDE_TEXCOORD0 + set); break;
			case 3: xgpu_text_append(&text, "\t\tt = vec4(v%lu.xyz, 1.0);\n", D3DVSDE_TEXCOORD0 + set); break;
			case 4: xgpu_text_append(&text, "\t\tt = v%lu;\n", D3DVSDE_TEXCOORD0 + set); break;
			default: xgpu_text_append(&text, "\t\tt = vec4(0.0, 0.0, 1.0, 0.0);\n"); break;
			}
			break;
		}
		if (transform_count)
		{
			xgpu_text_append(&text, "\t\tt = XFORM(t, %d);\n", D3D8_FF_TEXTURE + stage * 4);
			size = transform_count;
		}
		/* the pixel shader divides xyz by w (project2d): w is the divisor
		when projected, else 1 */
		if (projected && transform_count == 2)
			xgpu_text_append(&text, "\t\txT%d = vec4(t.x, 0.0, 0.0, t.y);\n", stage);
		else if (projected && transform_count == 3)
			xgpu_text_append(&text, "\t\txT%d = vec4(t.xy, 0.0, t.z);\n", stage);
		else if (projected && transform_count == 4)
			xgpu_text_append(&text, "\t\txT%d = t;\n", stage);
		else if (size <= 2)
			xgpu_text_append(&text, "\t\txT%d = vec4(t.xy, 0.0, 1.0);\n", stage);
		else
			xgpu_text_append(&text, "\t\txT%d = vec4(t.xyz, 1.0);\n", stage);
		xgpu_text_append(&text, "\t}\n");
	}

	switch (key->fog)
	{
	case 1: xgpu_text_append(&text, "\txFog = abs(P.z);\n"); break;
	case 2: xgpu_text_append(&text, "\txFog = length(P);\n"); break;
	case 3: xgpu_text_append(&text, "\txFog = color1.a;\n"); break;
	default: xgpu_text_append(&text, "\txFog = 1.0;\n"); break;
	}
	xgpu_text_append(&text,
		"\tif (!(abs(position.w) > 0.0))\n"
		"\t\tposition = vec4(0.0, 0.0, 0.0, -1.0);\n"
		"\tgl_Position = position;\n"
#ifdef HALO_GLES
		"\tgl_Position.y = -gl_Position.y;\n"
		"\tgl_Position.z = 2.0 * gl_Position.z - gl_Position.w;\n"
#endif
		"\tgl_PointSize = point_size;\n"
		"\txD0 = clamp(color0, 0.0, 1.0);\n"
		"\txD1 = clamp(color1, 0.0, 1.0);\n"
		"\txB0 = xD0;\n"
		"\txB1 = xD1;\n"
		"}\n");
	return text.buffer;
}

/* ---------- uniforms */

static void matrix_multiply(const D3DMATRIX *a, const D3DMATRIX *b, D3DMATRIX *out)
{
	const float (*x)[4] = (const float (*)[4])&a->_11;
	const float (*y)[4] = (const float (*)[4])&b->_11;
	float result[4][4];
	int row, column;

	for (row = 0; row < 4; row++)
	{
		for (column = 0; column < 4; column++)
		{
			result[row][column] = x[row][0] * y[0][column] + x[row][1] * y[1][column] +
				x[row][2] * y[2][column] + x[row][3] * y[3][column];
		}
	}
	memcpy(&out->_11, result, sizeof(result));
}

/* the rows of the inverse transpose of m's upper 3x3: what normals are
multiplied by (as row vectors) */
static void normal_matrix(const D3DMATRIX *m, float out[3][4])
{
	const float (*a)[4] = (const float (*)[4])&m->_11;
	float c[3][3], determinant;
	int row, column;

	/* cofactors */
	c[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];
	c[0][1] = a[1][2] * a[2][0] - a[1][0] * a[2][2];
	c[0][2] = a[1][0] * a[2][1] - a[1][1] * a[2][0];
	c[1][0] = a[0][2] * a[2][1] - a[0][1] * a[2][2];
	c[1][1] = a[0][0] * a[2][2] - a[0][2] * a[2][0];
	c[1][2] = a[0][1] * a[2][0] - a[0][0] * a[2][1];
	c[2][0] = a[0][1] * a[1][2] - a[0][2] * a[1][1];
	c[2][1] = a[0][2] * a[1][0] - a[0][0] * a[1][2];
	c[2][2] = a[0][0] * a[1][1] - a[0][1] * a[1][0];
	determinant = a[0][0] * c[0][0] + a[0][1] * c[0][1] + a[0][2] * c[0][2];
	if (fabsf(determinant) < 1.0e-30f)
		determinant = 1.0f;
	/* the inverse is the transposed cofactors over the determinant; its
	transpose is the cofactors themselves */
	for (row = 0; row < 3; row++)
	{
		for (column = 0; column < 3; column++)
			out[row][column] = c[row][column] / determinant;
		out[row][3] = 0.0f;
	}
}

static void color_value(const D3DCOLORVALUE *color, float *out)
{
	out[0] = color->r;
	out[1] = color->g;
	out[2] = color->b;
	out[3] = color->a;
}

static void transform_point(const D3DMATRIX *m, const D3DVECTOR *v, float w, float *out)
{
	const float (*a)[4] = (const float (*)[4])&m->_11;
	int column;

	for (column = 0; column < 3; column++)
		out[column] = v->x * a[0][column] + v->y * a[1][column] + v->z * a[2][column] + w * a[3][column];
	out[3] = 0.0f;
}

BOOL d3d8_ff_uniforms(const D3DMATRIX *transforms, float uniforms[D3D8_FF_UNIFORM_COUNT][4])
{
	static float last[D3D8_FF_UNIFORM_COUNT][4];
	static BOOL have_last;
	const D3DMATRIX *view = &transforms[D3DTS_VIEW];
	D3DMATRIX world_view;
	unsigned long indices[D3D8_FF_MAX_LIGHTS], count, index;
	DWORD ambient = D3D__RenderState[D3DRS_AMBIENT];
	int stage;

	memset(uniforms, 0, sizeof(float) * 4 * D3D8_FF_UNIFORM_COUNT);
	matrix_multiply(&transforms[D3DTS_WORLD], view, &world_view);
	memcpy(uniforms[D3D8_FF_WORLDVIEW], &world_view._11, 16 * sizeof(float));
	memcpy(uniforms[D3D8_FF_PROJECTION], &transforms[D3DTS_PROJECTION]._11, 16 * sizeof(float));
	normal_matrix(&world_view, &uniforms[D3D8_FF_NORMAL]);
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		memcpy(uniforms[D3D8_FF_TEXTURE + stage * 4], &transforms[D3DTS_TEXTURE0 + stage]._11, 16 * sizeof(float));
	color_value(&material.Diffuse, uniforms[D3D8_FF_MATERIAL + 0]);
	color_value(&material.Ambient, uniforms[D3D8_FF_MATERIAL + 1]);
	color_value(&material.Specular, uniforms[D3D8_FF_MATERIAL + 2]);
	color_value(&material.Emissive, uniforms[D3D8_FF_MATERIAL + 3]);
	uniforms[D3D8_FF_MATERIAL + 4][0] = material.Power;
	uniforms[D3D8_FF_AMBIENT][0] = (float)((ambient >> 16) & 0xff) / 255.0f;
	uniforms[D3D8_FF_AMBIENT][1] = (float)((ambient >> 8) & 0xff) / 255.0f;
	uniforms[D3D8_FF_AMBIENT][2] = (float)(ambient & 0xff) / 255.0f;
	uniforms[D3D8_FF_AMBIENT][3] = (float)((ambient >> 24) & 0xff) / 255.0f;

	count = active_lights(indices);
	for (index = 0; index < count; index++)
	{
		const D3DLIGHT8 *light = &lights[indices[index]].light;
		float (*u)[4] = &uniforms[D3D8_FF_LIGHTS + index * 7];
		float length;

		color_value(&light->Diffuse, u[0]);
		color_value(&light->Specular, u[1]);
		color_value(&light->Ambient, u[2]);
		transform_point(view, &light->Position, 1.0f, u[3]);
		transform_point(view, &light->Direction, 0.0f, u[4]);
		length = sqrtf(u[4][0] * u[4][0] + u[4][1] * u[4][1] + u[4][2] * u[4][2]);
		if (length > 0.0f)
		{
			u[4][0] /= length;
			u[4][1] /= length;
			u[4][2] /= length;
		}
		u[5][0] = light->Type == D3DLIGHT_DIRECTIONAL ? 3.0e38f : light->Range;
		u[5][1] = light->Falloff;
		u[5][2] = light->Attenuation0;
		u[5][3] = light->Attenuation1;
		u[6][0] = light->Attenuation2;
		u[6][1] = cosf(light->Theta * 0.5f);
		u[6][2] = cosf(light->Phi * 0.5f);
	}
	if (have_last && !memcmp(last, uniforms, sizeof(last)))
		return FALSE;
	memcpy(last, uniforms, sizeof(last));
	have_last = TRUE;
	return TRUE;
}

/* ---------- texture stages as register combiners */

enum
{
	_reg_zero = 0x0,
	_reg_c0 = 0x1,
	_reg_fog = 0x3,
	_reg_v0 = 0x4,
	_reg_v1 = 0x5,
	_reg_t0 = 0x8,
	_reg_r0 = 0xc,
	_reg_r1 = 0xd,
	_reg_v1r0_sum = 0xe,
};

#define INPUT_ALPHA 0x10
#define MAP_IDENTITY 0x00
#define MAP_INVERT 0x20
#define MAP_EXPAND 0x40
#define MAP_MASK 0xe0
/* output flags (outputs >> 12) */
#define OUT_CD_DOT 0x01
#define OUT_AB_DOT 0x02
#define OUT_BIAS 0x08
#define OUT_SHIFT1 0x10
#define OUT_SHIFT2 0x20
#define OUT_AB_BLUE_TO_ALPHA 0x80

#define ONE (_reg_zero | MAP_INVERT)
#define ZERO (_reg_zero | MAP_IDENTITY)
#define MINUS_ONE (_reg_zero | MAP_EXPAND)

/* what the stage has bound (d3d8_ff_combiners' texture_kind) */
static const unsigned char *stage_textures;

static unsigned long argument_input(int stage, DWORD argument, BOOL alpha_portion)
{
	unsigned long reg;

	switch (argument & D3DTA_SELECTMASK)
	{
	case D3DTA_DIFFUSE: reg = _reg_v0; break;
	/* the current color at stage 0 is the diffuse color */
	case D3DTA_CURRENT: reg = stage ? _reg_r0 : _reg_v0; break;
	case D3DTA_TEXTURE:
		/* a stage without a texture reads white (so the default stage 0,
		texture times diffuse, gives the diffuse color) */
		if (stage_textures && stage_textures[stage] == _d3d8_ff_texture_none)
			return (argument & D3DTA_COMPLEMENT) ? ZERO : ONE;
		reg = (unsigned long)(_reg_t0 + stage);
		break;
	case D3DTA_TFACTOR: reg = _reg_c0; break;
	case D3DTA_SPECULAR: reg = _reg_v1; break;
	case D3DTA_TEMP: reg = _reg_r1; break;
	default: reg = _reg_zero; break;
	}
	if (alpha_portion || (argument & D3DTA_ALPHAREPLICATE))
		reg |= INPUT_ALPHA;
	if (argument & D3DTA_COMPLEMENT)
		reg |= MAP_INVERT;
	return reg;
}

static unsigned long inverted(unsigned long input)
{
	unsigned long mapping = input & MAP_MASK;

	if (mapping == MAP_IDENTITY)
		return (input & ~MAP_MASK) | MAP_INVERT;
	if (mapping == MAP_INVERT)
		return input & ~MAP_MASK;
	return input;
}

static unsigned long expanded(unsigned long input)
{
	return (input & ~MAP_MASK) | MAP_EXPAND;
}

static BOOL missing_texture(int stage, DWORD argument)
{
	return (argument & D3DTA_SELECTMASK) == D3DTA_TEXTURE && stage_textures &&
		stage_textures[stage] == _d3d8_ff_texture_none;
}

/* whether the operation reads (with one of the arguments it uses, or its own
texture alpha) the texture of a stage that has none (0x001ff2c0/0x001fead0 return
1 for such an argument) */
static BOOL stage_reads_missing_texture(int stage, DWORD op, BOOL alpha)
{
	const DWORD *ts = D3D__TextureState[stage];
	DWORD arg0 = ts[alpha ? D3DTSS_ALPHAARG0 : D3DTSS_COLORARG0];
	DWORD arg1 = ts[alpha ? D3DTSS_ALPHAARG1 : D3DTSS_COLORARG1];
	DWORD arg2 = ts[alpha ? D3DTSS_ALPHAARG2 : D3DTSS_COLORARG2];

	switch (op)
	{
	case D3DTOP_DISABLE:
		return FALSE;
	case D3DTOP_SELECTARG1:
		return missing_texture(stage, arg1);
	case D3DTOP_SELECTARG2:
		return missing_texture(stage, arg2);
	case D3DTOP_BLENDTEXTUREALPHA:
	case D3DTOP_BLENDTEXTUREALPHAPM:
		return missing_texture(stage, D3DTA_TEXTURE);
	case D3DTOP_MULTIPLYADD:
	case D3DTOP_LERP:
		return missing_texture(stage, arg0) || missing_texture(stage, arg1) || missing_texture(stage, arg2);
	case D3DTOP_PREMODULATE:
	case D3DTOP_BUMPENVMAP:
	case D3DTOP_BUMPENVMAPLUMINANCE:
		/* (passed on in any case, see stage_portion) */
		return FALSE;
	default:
		return missing_texture(stage, arg1) || missing_texture(stage, arg2);
	}
}

/* one portion (color or alpha) of one stage: its inputs word and outputs
word, or FALSE if the stage leaves this portion alone */
static BOOL stage_portion(int stage, BOOL alpha, DWORD *inputs, DWORD *outputs)
{
	const DWORD *ts = D3D__TextureState[stage];
	DWORD op = ts[alpha ? D3DTSS_ALPHAOP : D3DTSS_COLOROP];
	unsigned long arg0 = argument_input(stage, ts[alpha ? D3DTSS_ALPHAARG0 : D3DTSS_COLORARG0], alpha);
	unsigned long arg1 = argument_input(stage, ts[alpha ? D3DTSS_ALPHAARG1 : D3DTSS_COLORARG1], alpha);
	unsigned long arg2 = argument_input(stage, ts[alpha ? D3DTSS_ALPHAARG2 : D3DTSS_COLORARG2], alpha);
	unsigned long current = (stage ? _reg_r0 : _reg_v0) | INPUT_ALPHA;
	unsigned long a = ZERO, b = ZERO, c = ZERO, d = ZERO, flags = 0;
	unsigned long result = ts[D3DTSS_RESULTARG] == D3DTA_TEMP ? _reg_r1 : _reg_r0;
	BOOL dot = FALSE;

	switch (op)
	{
	case D3DTOP_DISABLE:
		/* (only the alpha can be disabled while the color is not: it
		passes on) */
		a = argument_input(stage, D3DTA_CURRENT, alpha);
		b = ONE;
		break;
	case D3DTOP_SELECTARG1: a = arg1; b = ONE; break;
	case D3DTOP_SELECTARG2: a = arg2; b = ONE; break;
	case D3DTOP_MODULATE: a = arg1; b = arg2; break;
	case D3DTOP_MODULATE2X: a = arg1; b = arg2; flags = OUT_SHIFT1; break;
	case D3DTOP_MODULATE4X: a = arg1; b = arg2; flags = OUT_SHIFT2; break;
	case D3DTOP_ADD: a = arg1; b = ONE; c = arg2; d = ONE; break;
	case D3DTOP_ADDSIGNED: a = arg1; b = ONE; c = arg2; d = ONE; flags = OUT_BIAS; break;
	case D3DTOP_ADDSIGNED2X: a = arg1; b = ONE; c = arg2; d = ONE; flags = OUT_BIAS | OUT_SHIFT1; break;
	case D3DTOP_SUBTRACT: a = arg1; b = ONE; c = arg2; d = MINUS_ONE; break;
	case D3DTOP_ADDSMOOTH: a = arg1; b = ONE; c = arg2; d = inverted(arg1); break;
	case D3DTOP_BLENDDIFFUSEALPHA:
		a = arg1; b = _reg_v0 | INPUT_ALPHA; c = arg2; d = inverted(_reg_v0 | INPUT_ALPHA);
		break;
	case D3DTOP_BLENDCURRENTALPHA:
		a = arg1; b = current; c = arg2; d = inverted(current);
		break;
	case D3DTOP_BLENDTEXTUREALPHA:
		b = argument_input(stage, D3DTA_TEXTURE, TRUE);
		a = arg1; c = arg2; d = inverted(b);
		break;
	case D3DTOP_BLENDFACTORALPHA:
		a = arg1; b = _reg_c0 | INPUT_ALPHA; c = arg2; d = inverted(_reg_c0 | INPUT_ALPHA);
		break;
	case D3DTOP_BLENDTEXTUREALPHAPM:
		a = arg1; b = ONE; c = arg2; d = inverted(argument_input(stage, D3DTA_TEXTURE, TRUE));
		break;
	case D3DTOP_MODULATEALPHA_ADDCOLOR:
		/* rgb = arg1 + arg1.a * arg2 */
		a = arg1; b = ONE; c = arg1 | INPUT_ALPHA; d = arg2;
		break;
	case D3DTOP_MODULATECOLOR_ADDALPHA: a = arg1; b = arg2; c = arg1 | INPUT_ALPHA; d = ONE; break;
	case D3DTOP_MODULATEINVALPHA_ADDCOLOR: a = inverted(arg1 | INPUT_ALPHA); b = arg2; c = arg1; d = ONE; break;
	case D3DTOP_MODULATEINVCOLOR_ADDALPHA: a = inverted(arg1); b = arg2; c = arg1 | INPUT_ALPHA; d = ONE; break;
	case D3DTOP_DOTPRODUCT3:
		if (alpha)
		{
			/* (as an alpha operation alone: the dot product's value) */
			a = expanded(arg1); b = expanded(arg2);
			break;
		}
		a = expanded(arg1); b = expanded(arg2);
		flags = OUT_AB_DOT | OUT_AB_BLUE_TO_ALPHA;
		dot = TRUE;
		break;
	case D3DTOP_MULTIPLYADD: a = arg1; b = arg2; c = arg0; d = ONE; break;
	case D3DTOP_LERP: a = arg0; b = arg1; c = inverted(arg0); d = arg2; break;
	default:
		/* PREMODULATE and the bump environment operations (whose texture
		mode the next stage takes) pass the current color on */
		a = argument_input(stage, D3DTA_CURRENT, alpha);
		b = ONE;
		break;
	}
	*inputs = (DWORD)((a << 24) | (b << 16) | (c << 8) | d);
	if (stage_reads_missing_texture(stage, op, alpha))
	{
		/* as the XBE's D3D (0x001ff9d0 color, 0x001ff030 alpha): an operation that reads
		the texture of a stage without one passes the stage's input on (the diffuse color
		at stage 0, the current color after it); the output flags stay */
		*inputs = (DWORD)((((stage ? _reg_r0 : _reg_v0) | (alpha ? INPUT_ALPHA : 0)) << 24) | (ONE << 16));
	}
	if (dot)
		*outputs = (DWORD)((flags << 12) | (result << 4));
	else
		*outputs = (DWORD)((flags << 12) | (result << 8));
	return TRUE;
}

static unsigned long texture_mode(unsigned char kind)
{
	switch (kind)
	{
	case _d3d8_ff_texture_2d: return 0x01;   /* project2d */
	case _d3d8_ff_texture_3d: return 0x02;   /* project3d */
	case _d3d8_ff_texture_cube: return 0x03; /* cubemap */
	default: return 0x00;
	}
}

void d3d8_ff_combiners(const unsigned char texture_kind[D3DTSS_MAXSTAGES], DWORD state[D3DRS_PS_MAX],
	DWORD *texture_modes)
{
	DWORD tfactor = D3D__RenderState[D3DRS_TEXTUREFACTOR];
	unsigned long count = 0, final_a, final_b, final_c;
	DWORD modes = 0;
	int stage;

	memset(state, 0, sizeof(DWORD) * D3DRS_PS_MAX);
	stage_textures = texture_kind;
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		const DWORD *ts = D3D__TextureState[stage];
		DWORD inputs, outputs;

		if (ts[D3DTSS_COLOROP] == D3DTOP_DISABLE || ts[D3DTSS_COLOROP] == 0)
			break;
		if (stage && (D3D__TextureState[stage - 1][D3DTSS_COLOROP] == D3DTOP_BUMPENVMAP ||
			D3D__TextureState[stage - 1][D3DTSS_COLOROP] == D3DTOP_BUMPENVMAPLUMINANCE))
		{
			modes |= (DWORD)((D3D__TextureState[stage - 1][D3DTSS_COLOROP] == D3DTOP_BUMPENVMAP ? 0x06 : 0x07)
				<< (5 * stage));
		}
		else
		{
			modes |= (DWORD)(texture_mode(texture_kind[stage]) << (5 * stage));
		}
		if (stage_portion(stage, FALSE, &inputs, &outputs))
		{
			state[D3DRS_PSRGBINPUTS0 + stage] = inputs;
			state[D3DRS_PSRGBOUTPUTS0 + stage] = outputs;
		}
		/* a dot product color goes to alpha too, whatever the alpha
		operation (the color portion writes it: OUT_AB_BLUE_TO_ALPHA) */
		if (ts[D3DTSS_COLOROP] != D3DTOP_DOTPRODUCT3 && stage_portion(stage, TRUE, &inputs, &outputs))
		{
			state[D3DRS_PSALPHAINPUTS0 + stage] = inputs;
			state[D3DRS_PSALPHAOUTPUTS0 + stage] = outputs;
		}
		count++;
	}
	if (!count)
	{
		/* no stage: the diffuse color */
		state[D3DRS_PSRGBINPUTS0] = (DWORD)((_reg_v0 << 24) | (ONE << 16));
		state[D3DRS_PSRGBOUTPUTS0] = (DWORD)(_reg_r0 << 8);
		state[D3DRS_PSALPHAINPUTS0] = (DWORD)(((_reg_v0 | INPUT_ALPHA) << 24) | (ONE << 16));
		state[D3DRS_PSALPHAOUTPUTS0] = (DWORD)(_reg_r0 << 8);
		count = 1;
	}
	state[D3DRS_PSCOMBINERCOUNT] = (DWORD)count;
	for (stage = 0; stage < 8; stage++)
		state[D3DRS_PSCONSTANT0_0 + stage] = tfactor;

	/* the final combiner: the specular color added, then fog:
	rgb = A * B + (1 - A) * C + D, alpha = G */
	final_b = D3D__RenderState[D3DRS_SPECULARENABLE] ? _reg_v1r0_sum : _reg_r0;
	if (D3D__RenderState[D3DRS_FOGENABLE])
	{
		final_a = _reg_fog | INPUT_ALPHA;
		final_c = _reg_fog;
	}
	else
	{
		final_a = ONE;
		final_c = ZERO;
	}
	state[D3DRS_PSFINALCOMBINERINPUTSABCD] = (DWORD)((final_a << 24) | (final_b << 16) | (final_c << 8) | ZERO);
	/* G = r0's alpha; 0x80 clamps the specular sum */
	state[D3DRS_PSFINALCOMBINERINPUTSEFG] = (DWORD)(((_reg_r0 | INPUT_ALPHA) << 8) | 0x80);
	*texture_modes = modes;
}
