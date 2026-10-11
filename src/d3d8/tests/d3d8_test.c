/*
D3D8_TEST.C

Tests of the d3d8 platform module (src/d3d8), run as a guest image in the
host (src/d3d8/tests/run_tests.sh builds and runs it):

1. FVF layouts: the vertex sizes the game's FVF codes need (its
   DrawVerticesUP strides are 0x1c and 0x24).
2. Texture stages as combiners: each D3DTOP the game can use is evaluated
   on a CPU model of the NV2A combiners (the semantics nv2a_psh.c turns
   into GLSL) and compared with the Direct3D 8 formula for that operation.
3. XGAssembleShader: shaders like the game's (bActor baGenerateShader) are
   assembled, run on a CPU model of the vertex program decoding nv2a_vsh.c
   does, and compared with the math the text asks for, including the
   screen-space conversion the XDK appends. Bad text fails with a message
   and still returns every buffer (the game releases them unchecked).
4. XGSetTextureHeader: the header the game's 640x480 X8R8G8B8 screen copy
   gets describes that texture.
5. With a window (not SDL_VIDEO_DRIVER=dummy): the device draws an FVF
   quad with DrawVerticesUP through the fixed-function shaders, and the
   pixels read back are the vertex color, modulated by a texture factor
   stage, and lit by a directional light.

Exit code 0 when all pass; otherwise the number of failed checks.
*/

#include "d3d8_ff.h"
#include "sdl_platform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do { checks++; if (!(condition)) { failures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); printf("\n"); } } while (0)

static int near(float a, float b, float tolerance)
{
	return fabsf(a - b) <= tolerance;
}

/* ---------- 1. FVF layouts */

static void test_fvf(void)
{
	struct d3d8_ff_element elements[16];
	unsigned long stride, count;

	/* XYZRHW | DIFFUSE | TEX1: the game's 2D quads (stride 0x1c) */
	count = d3d8_ff_fvf_elements(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, elements, 16, &stride);
	CHECK(count == 3 && stride == 0x1c, "FVF 0x144: %lu elements, stride %lu", count, stride);
	CHECK(elements[0].reg == D3DVSDE_POSITION && elements[0].type == D3DVSDT_FLOAT4 && elements[0].offset == 0,
		"FVF 0x144 position");
	CHECK(elements[1].reg == D3DVSDE_DIFFUSE && elements[1].type == D3DVSDT_D3DCOLOR && elements[1].offset == 16,
		"FVF 0x144 diffuse");
	CHECK(elements[2].reg == D3DVSDE_TEXCOORD0 && elements[2].type == D3DVSDT_FLOAT2 && elements[2].offset == 20,
		"FVF 0x144 texture");
	/* XYZ | NORMAL | DIFFUSE | TEX1 (0x152): stride 0x24 */
	count = d3d8_ff_fvf_elements(0x152, elements, 16, &stride);
	CHECK(count == 4 && stride == 0x24, "FVF 0x152: %lu elements, stride %lu", count, stride);
	CHECK(elements[1].reg == D3DVSDE_NORMAL && elements[1].offset == 12, "FVF 0x152 normal");
	/* texture coordinate sizes */
	count = d3d8_ff_fvf_elements(D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE1(1),
		elements, 16, &stride);
	CHECK(count == 3 && stride == 12 + 12 + 4 && elements[1].type == D3DVSDT_FLOAT3 &&
		elements[2].type == D3DVSDT_FLOAT1 && elements[2].reg == D3DVSDE_TEXCOORD1, "texture coordinate sizes");
}

/* ---------- 1b. fixed-function uniforms: recomputed only when an entry
point changed their inputs (a missed change would draw with stale
matrices, lights or material; a skipped compare would upload them on
every draw) */

static void load_transforms(D3DMATRIX transforms[D3DTS_MAX])
{
	int index;

	for (index = 0; index < D3DTS_MAX; index++)
		D3DDevice_GetTransform((D3DTRANSFORMSTATETYPE)index, &transforms[index]);
}

static void test_ff_uniforms(void)
{
	static float uniforms[D3D8_FF_UNIFORM_COUNT][4];
	static float other[D3D8_FF_UNIFORM_COUNT][4];
	D3DMATRIX transforms[D3DTS_MAX], identity, moved;
	D3DMATERIAL8 material, saved_material = { { 1.0f, 1.0f, 1.0f, 1.0f } };
	D3DLIGHT8 light;

	memset(&identity, 0, sizeof(identity));
	identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
	D3DDevice_SetTransform(D3DTS_WORLD, &identity);
	D3DDevice_SetTransform(D3DTS_VIEW, &identity);
	load_transforms(transforms);
	d3d8_ff_uniforms(transforms, uniforms);
	CHECK(!d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: no change, no update");

	moved = identity;
	moved._41 = 5.0f;
	D3DDevice_SetTransform(D3DTS_WORLD, &moved);
	load_transforms(transforms);
	CHECK(d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: SetTransform(WORLD) updates them");
	CHECK(uniforms[D3D8_FF_WORLDVIEW + 3][0] == 5.0f, "ff uniforms: world-view row 3 x = %f, expected 5",
		uniforms[D3D8_FF_WORLDVIEW + 3][0]);
	D3DDevice_SetTransform(D3DTS_WORLD, &moved);
	CHECK(!d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: the same matrix again, no update");

	memset(&material, 0, sizeof(material));
	material.Diffuse.r = 0.25f;
	material.Power = 8.0f;
	D3DDevice_SetMaterial(&material);
	CHECK(d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: SetMaterial updates them");
	CHECK(uniforms[D3D8_FF_MATERIAL][0] == 0.25f && uniforms[D3D8_FF_MATERIAL + 4][0] == 8.0f,
		"ff uniforms: material diffuse %f, power %f", uniforms[D3D8_FF_MATERIAL][0], uniforms[D3D8_FF_MATERIAL + 4][0]);

	memset(&light, 0, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.g = 0.5f;
	light.Direction.x = 1.0f;
	D3DDevice_SetLight(0, &light);
	D3DDevice_LightEnable(0, TRUE);
	CHECK(d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: SetLight and LightEnable update them");
	CHECK(uniforms[D3D8_FF_LIGHTS][1] == 0.5f && uniforms[D3D8_FF_LIGHTS + 4][0] == 1.0f,
		"ff uniforms: light diffuse g %f, direction x %f", uniforms[D3D8_FF_LIGHTS][1], uniforms[D3D8_FF_LIGHTS + 4][0]);

	/* lights are in camera space: a new view turns their direction */
	memset(&moved, 0, sizeof(moved));
	moved._12 = 1.0f;
	moved._21 = -1.0f;
	moved._33 = moved._44 = 1.0f;
	D3DDevice_SetTransform(D3DTS_VIEW, &moved);
	load_transforms(transforms);
	CHECK(d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: SetTransform(VIEW) updates them");
	CHECK(near(uniforms[D3D8_FF_LIGHTS + 4][1], 1.0f, 1e-6f), "ff uniforms: light direction in camera space y %f",
		uniforms[D3D8_FF_LIGHTS + 4][1]);
	CHECK(d3d8_ff_uniforms_check(transforms, (const float (*)[4])uniforms), "ff uniforms: equal a full computation");

	D3DDevice_LightEnable(0, FALSE);
	CHECK(d3d8_ff_uniforms(transforms, uniforms), "ff uniforms: LightEnable(FALSE) updates them");
	CHECK(uniforms[D3D8_FF_LIGHTS][1] == 0.0f, "ff uniforms: a disabled light's rows are zero");

	/* another array (the device's, for the drawing tests) is filled whole */
	CHECK(d3d8_ff_uniforms(transforms, other), "ff uniforms: another array is filled");
	CHECK(!memcmp(other, uniforms, sizeof(other)), "ff uniforms: another array gets every row");

	D3DDevice_SetTransform(D3DTS_WORLD, &identity);
	D3DDevice_SetTransform(D3DTS_VIEW, &identity);
	D3DDevice_SetMaterial(&saved_material);
}

/* ---------- 2. CPU model of the NV2A register combiners (nv2a_psh.c) */

struct combiner_inputs
{
	float v0[4], v1[4], t[4][4], fog[4];
};

static float clampf(float x, float low, float high)
{
	return x < low ? low : x > high ? high : x;
}

static void register_value(float regs[16][4], unsigned long input, int alpha_portion, float out[3])
{
	unsigned long reg = input & 0xf;
	int alpha_channel = (input & 0x10) != 0;
	unsigned long mapping = input & 0xe0;
	int component;

	for (component = 0; component < 3; component++)
	{
		float x = alpha_portion ? regs[reg][alpha_channel ? 3 : 2] : regs[reg][alpha_channel ? 3 : component];

		switch (mapping)
		{
		case 0x00: x = fmaxf(x, 0.0f); break;
		case 0x20: x = 1.0f - clampf(x, 0.0f, 1.0f); break;
		case 0x40: x = 2.0f * fmaxf(x, 0.0f) - 1.0f; break;
		case 0x60: x = 1.0f - 2.0f * fmaxf(x, 0.0f); break;
		case 0x80: x = fmaxf(x, 0.0f) - 0.5f; break;
		case 0xa0: x = 0.5f - fmaxf(x, 0.0f); break;
		case 0xc0: break;
		default: x = -x; break;
		}
		out[component] = x;
	}
}

static float output_map(float x, unsigned long flags)
{
	switch (flags & 0x38)
	{
	case 0x08: x -= 0.5f; break;
	case 0x10: x *= 2.0f; break;
	case 0x18: x = (x - 0.5f) * 2.0f; break;
	case 0x20: x *= 4.0f; break;
	case 0x30: x *= 0.5f; break;
	default: break;
	}
	return clampf(x, -1.0f, 1.0f);
}

static void evaluate_combiners(const DWORD *state, const struct combiner_inputs *in, float result[4])
{
	float regs[16][4];
	unsigned long stages = state[D3DRS_PSCOMBINERCOUNT] & 0xff, stage;
	int portion, i;

	memset(regs, 0, sizeof(regs));
	memcpy(regs[4], in->v0, sizeof(regs[4]));
	memcpy(regs[5], in->v1, sizeof(regs[5]));
	for (i = 0; i < 4; i++)
		memcpy(regs[8 + i], in->t[i], sizeof(regs[8]));
	memcpy(regs[3], in->fog, sizeof(regs[3]));
	regs[12][3] = in->t[0][3];
	for (stage = 0; stage < stages; stage++)
	{
		float ab[2][3], cd[2][3], sum[2][3];
		unsigned long outputs[2], flags[2];

		/* the stage's constant c0 (one for all stages: no unique flag) */
		for (i = 0; i < 4; i++)
		{
			DWORD c = state[D3DRS_PSCONSTANT0_0];
			int shift = i == 3 ? 24 : 16 - 8 * i;

			regs[1][i] = (float)((c >> shift) & 0xff) / 255.0f;
		}
		for (portion = 0; portion < 2; portion++)
		{
			DWORD inputs = portion ? state[D3DRS_PSALPHAINPUTS0 + stage] : state[D3DRS_PSRGBINPUTS0 + stage];
			float a[3], b[3], c[3], d[3];
			int component;

			outputs[portion] = portion ? state[D3DRS_PSALPHAOUTPUTS0 + stage] : state[D3DRS_PSRGBOUTPUTS0 + stage];
			flags[portion] = outputs[portion] >> 12;
			register_value(regs, (inputs >> 24) & 0xff, portion, a);
			register_value(regs, (inputs >> 16) & 0xff, portion, b);
			register_value(regs, (inputs >> 8) & 0xff, portion, c);
			register_value(regs, inputs & 0xff, portion, d);
			for (component = 0; component < 3; component++)
			{
				ab[portion][component] = (!portion && (flags[portion] & 2)) ?
					a[0] * b[0] + a[1] * b[1] + a[2] * b[2] : a[component] * b[component];
				cd[portion][component] = (!portion && (flags[portion] & 1)) ?
					c[0] * d[0] + c[1] * d[1] + c[2] * d[2] : c[component] * d[component];
				sum[portion][component] = ab[portion][component] + cd[portion][component];
				ab[portion][component] = output_map(ab[portion][component], flags[portion]);
				cd[portion][component] = output_map(cd[portion][component], flags[portion]);
				sum[portion][component] = output_map(sum[portion][component], flags[portion]);
			}
		}
		for (portion = 0; portion < 2; portion++)
		{
			unsigned long ab_reg = (outputs[portion] >> 4) & 0xf, cd_reg = outputs[portion] & 0xf;
			unsigned long sum_reg = (outputs[portion] >> 8) & 0xf;

			if (portion == 0)
			{
				if (ab_reg)
				{
					memcpy(regs[ab_reg], ab[0], 3 * sizeof(float));
					if (flags[0] & 0x80)
						regs[ab_reg][3] = ab[0][2];
				}
				if (cd_reg)
					memcpy(regs[cd_reg], cd[0], 3 * sizeof(float));
				if (sum_reg)
					memcpy(regs[sum_reg], sum[0], 3 * sizeof(float));
			}
			else
			{
				if (ab_reg)
					regs[ab_reg][3] = ab[1][0];
				if (cd_reg)
					regs[cd_reg][3] = cd[1][0];
				if (sum_reg)
					regs[sum_reg][3] = sum[1][0];
			}
		}
	}
	{
		DWORD abcd = state[D3DRS_PSFINALCOMBINERINPUTSABCD], efg = state[D3DRS_PSFINALCOMBINERINPUTSEFG];
		float fa[3], fb[3], fc[3], fd[3], fg[3];
		int component;

		if (!abcd && !efg)
		{
			memcpy(result, regs[12], 4 * sizeof(float));
			return;
		}
		for (component = 0; component < 3; component++)
		{
			regs[14][component] = clampf(regs[5][component], 0.0f, 1.0f) + clampf(regs[12][component], 0.0f, 1.0f);
			if (efg & 0x80)
				regs[14][component] = clampf(regs[14][component], 0.0f, 1.0f);
		}
#define FINAL_INPUT(input, alpha, out) \
		do { float raw[3]; int k; register_value(regs, (input) & 0x1f, alpha, raw); \
			for (k = 0; k < 3; k++) { float x = alpha ? regs[(input) & 0xf][((input) & 0x10) ? 3 : 2] : \
				regs[(input) & 0xf][((input) & 0x10) ? 3 : k]; x = clampf(x, 0.0f, 1.0f); \
				(out)[k] = ((input) & 0x20) ? 1.0f - x : x; } (void)raw; } while (0)
		FINAL_INPUT((abcd >> 24) & 0xff, 0, fa);
		FINAL_INPUT((abcd >> 16) & 0xff, 0, fb);
		FINAL_INPUT((abcd >> 8) & 0xff, 0, fc);
		FINAL_INPUT(abcd & 0xff, 0, fd);
		FINAL_INPUT((efg >> 8) & 0xff, 1, fg);
#undef FINAL_INPUT
		for (component = 0; component < 3; component++)
			result[component] = clampf(fa[component] * fb[component] + (1.0f - fa[component]) * fc[component] +
				fd[component], 0.0f, 1.0f);
		result[3] = clampf(fg[0], 0.0f, 1.0f);
	}
}

static void stage_state(int stage, DWORD color_op, DWORD color1, DWORD color2, DWORD alpha_op, DWORD alpha1,
	DWORD alpha2)
{
	D3D__TextureState[stage][D3DTSS_COLOROP] = color_op;
	D3D__TextureState[stage][D3DTSS_COLORARG1] = color1;
	D3D__TextureState[stage][D3DTSS_COLORARG2] = color2;
	D3D__TextureState[stage][D3DTSS_ALPHAOP] = alpha_op;
	D3D__TextureState[stage][D3DTSS_ALPHAARG1] = alpha1;
	D3D__TextureState[stage][D3DTSS_ALPHAARG2] = alpha2;
	D3D__TextureState[stage][D3DTSS_RESULTARG] = D3DTA_CURRENT;
}

static void stages_reset(void)
{
	int stage;

	memset(D3D__TextureState, 0, sizeof(D3D__TextureState));
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		stage_state(stage, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT);
	D3D__RenderState[D3DRS_FOGENABLE] = FALSE;
	D3D__RenderState[D3DRS_SPECULARENABLE] = FALSE;
	D3D__RenderState[D3DRS_TEXTUREFACTOR] = 0xffffffff;
}

static void run_stages(const unsigned char *kinds, const struct combiner_inputs *in, float result[4], DWORD *modes)
{
	static const unsigned char all_2d[4] = { 1, 1, 1, 1 };
	DWORD state[D3DRS_PS_MAX], texture_modes;

	d3d8_ff_combiners(kinds ? kinds : all_2d, state, &texture_modes);
	evaluate_combiners(state, in, result);
	if (modes)
		*modes = texture_modes;
}

#define CHECK_COLOR(result, r, g, b, a, what) \
	CHECK(near(result[0], r, 0.004f) && near(result[1], g, 0.004f) && near(result[2], b, 0.004f) && \
		near(result[3], a, 0.004f), "%s: got (%.3f %.3f %.3f %.3f), expected (%.3f %.3f %.3f %.3f)", what, \
		result[0], result[1], result[2], result[3], (double)(r), (double)(g), (double)(b), (double)(a))

static void test_combiners(void)
{
	struct combiner_inputs in;
	float result[4];
	DWORD modes;
	unsigned char kinds[4] = { _d3d8_ff_texture_2d, _d3d8_ff_texture_cube, 0, 0 };
	static const unsigned char none[4] = { 0, 0, 0, 0 };

	memset(&in, 0, sizeof(in));
	in.v0[0] = 0.5f; in.v0[1] = 1.0f; in.v0[2] = 0.5f; in.v0[3] = 0.3f;
	in.v1[0] = 0.25f; in.v1[1] = 0.0f; in.v1[2] = 0.5f; in.v1[3] = 1.0f;
	in.t[0][0] = 0.5f; in.t[0][1] = 0.25f; in.t[0][2] = 1.0f; in.t[0][3] = 0.8f;
	in.t[1][0] = 0.2f; in.t[1][1] = 0.4f; in.t[1][2] = 0.6f; in.t[1][3] = 0.25f;

	/* texture * diffuse, alpha from the texture */
	stages_reset();
	stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(kinds, &in, result, &modes);
	CHECK_COLOR(result, 0.25f, 0.25f, 0.5f, 0.8f, "MODULATE");
	CHECK((modes & 0x1f) == 1 && ((modes >> 5) & 0x1f) == 0, "texture modes 0x%lx: stage 0 project2d, stage 1 off",
		(unsigned long)modes);

	/* the second stage's cube map gets its mode once the stage is on */
	stage_state(1, D3DTOP_ADD, D3DTA_CURRENT, D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
	run_stages(kinds, &in, result, &modes);
	CHECK(((modes >> 5) & 0x1f) == 3, "stage 1 cube map mode, got 0x%lx", (unsigned long)modes);
	CHECK_COLOR(result, 0.45f, 0.65f, 1.0f, 0.2f, "MODULATE then ADD (clamped), alpha modulated");

	/* MODULATE2X and 4X saturate */
	stages_reset();
	stage_state(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 0.5f, 1.0f, 0.3f, "MODULATE2X, alpha SELECTARG2");
	stage_state(0, D3DTOP_MODULATE4X, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 1.0f, 1.0f, 1.0f, 0.3f, "MODULATE4X");

	/* ADDSIGNED: a + b - 0.5 */
	stage_state(0, D3DTOP_ADDSIGNED, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 0.75f, 1.0f, 0.8f, "ADDSIGNED");

	/* SUBTRACT clamps at 0 */
	stage_state(0, D3DTOP_SUBTRACT, D3DTA_DIFFUSE, D3DTA_TEXTURE, D3DTOP_SUBTRACT, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.0f, 0.75f, 0.0f, 0.5f, "SUBTRACT");

	/* BLENDTEXTUREALPHA: t * t.a + d * (1 - t.a) */
	stage_state(0, D3DTOP_BLENDTEXTUREALPHA, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE, D3DTA_TEXTURE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f * 0.8f + 0.5f * 0.2f, 0.25f * 0.8f + 0.2f, 0.8f + 0.5f * 0.2f, 0.3f, "BLENDTEXTUREALPHA");

	/* BLENDDIFFUSEALPHA and the complement modifier */
	stage_state(0, D3DTOP_BLENDDIFFUSEALPHA, D3DTA_TEXTURE, D3DTA_DIFFUSE | D3DTA_COMPLEMENT, D3DTOP_SELECTARG1,
		D3DTA_TEXTURE | D3DTA_COMPLEMENT, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f * 0.3f + 0.5f * 0.7f, 0.25f * 0.3f, 0.3f + 0.5f * 0.7f, 0.2f,
		"BLENDDIFFUSEALPHA with COMPLEMENT");

	/* BLENDFACTORALPHA with the texture factor */
	D3D__RenderState[D3DRS_TEXTUREFACTOR] = 0x40ff8000; /* a 0.25, r 1, g 0.5, b 0 */
	stage_state(0, D3DTOP_BLENDFACTORALPHA, D3DTA_TFACTOR, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TFACTOR, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.25f + 0.5f * 0.75f, 0.502f * 0.25f + 0.75f, 0.5f * 0.75f, 0.251f, "BLENDFACTORALPHA");

	/* ADDSMOOTH: a + b (1 - a) */
	stage_state(0, D3DTOP_ADDSMOOTH, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.75f, 1.0f, 1.0f, 0.8f, "ADDSMOOTH");

	/* MODULATEALPHA_ADDCOLOR: a.rgb + a.a * b.rgb */
	stage_state(0, D3DTOP_MODULATEALPHA_ADDCOLOR, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE,
		D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.9f, 1.0f, 1.0f, 0.8f, "MODULATEALPHA_ADDCOLOR");

	/* ALPHAREPLICATE */
	stage_state(0, D3DTOP_SELECTARG1, D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1,
		D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.8f, 0.8f, 0.8f, 0.8f, "ALPHAREPLICATE");

	/* MULTIPLYADD: arg0 + arg1 * arg2; LERP: arg0 * arg1 + (1 - arg0) * arg2 */
	stage_state(0, D3DTOP_MULTIPLYADD, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE, D3DTA_DIFFUSE);
	D3D__TextureState[0][D3DTSS_COLORARG0] = D3DTA_SPECULAR;
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 0.25f, 1.0f, 0.3f, "MULTIPLYADD");
	stage_state(0, D3DTOP_LERP, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE, D3DTA_DIFFUSE);
	D3D__TextureState[0][D3DTSS_COLORARG0] = D3DTA_SPECULAR;
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.25f * 0.5f + 0.75f * 0.5f, 1.0f, 0.5f * 1.0f + 0.5f * 0.5f, 0.3f, "LERP");

	/* DOTPRODUCT3 of signed vectors, replicated to alpha */
	in.t[0][0] = 1.0f; in.t[0][1] = 0.5f; in.t[0][2] = 0.5f;
	in.v0[0] = 0.75f; in.v0[1] = 0.5f; in.v0[2] = 1.0f;
	stage_state(0, D3DTOP_DOTPRODUCT3, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 0.5f, 0.5f, 0.5f, "DOTPRODUCT3");
	in.t[0][0] = 0.5f; in.t[0][1] = 0.25f; in.t[0][2] = 1.0f;
	in.v0[0] = 0.5f; in.v0[1] = 1.0f; in.v0[2] = 0.5f;

	/* a TEMP result read by the next stage */
	stages_reset();
	stage_state(0, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	D3D__TextureState[0][D3DTSS_RESULTARG] = D3DTA_TEMP;
	stage_state(1, D3DTOP_MODULATE, D3DTA_TEMP, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEMP, D3DTA_DIFFUSE);
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.25f, 0.25f, 0.5f, 0.8f, "RESULTARG TEMP");

	/* stage 0 off: the diffuse color */
	stages_reset();
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 1.0f, 0.5f, 0.3f, "no stage");

	/* the default stage 0 (texture * current) without a texture: diffuse (the XBE's D3D passes
	the diffuse color and alpha on when a stage reads a texture it does not have) */
	stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT);
	run_stages(none, &in, result, &modes);
	CHECK_COLOR(result, 0.5f, 1.0f, 0.5f, 0.3f, "MODULATE without a texture");
	CHECK(modes == 0, "no texture: no texture mode");
	stage_state(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	run_stages(none, &in, result, NULL);
	CHECK_COLOR(result, 1.0f, 1.0f, 1.0f, 0.3f, "MODULATE2X without a texture: diffuse, still doubled");

	/* Hopper variation 2 leaves stage 1 at BLENDFACTORALPHA (texture, current) while it draws
	untextured on stage 1 (players, floor lines): the stage must pass the current color on,
	not blend toward white (the white players of integration round 1) */
	stages_reset();
	stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	stage_state(1, D3DTOP_BLENDFACTORALPHA, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG2, D3DTA_TEXTURE,
		D3DTA_CURRENT);
	D3D__RenderState[D3DRS_TEXTUREFACTOR] = 0xff000000;
	kinds[1] = _d3d8_ff_texture_none;
	run_stages(kinds, &in, result, &modes);
	CHECK_COLOR(result, 0.25f, 0.25f, 0.5f, 0.24f, "stage 1 reading a missing texture passes the current color");
	CHECK(((modes >> 5) & 0x1f) == 0, "stage 1 without a texture: no texture mode, got 0x%lx", (unsigned long)modes);
	kinds[1] = _d3d8_ff_texture_2d;
	run_stages(kinds, &in, result, NULL);
	CHECK_COLOR(result, 0.2f, 0.4f, 0.6f, 0.24f, "BLENDFACTORALPHA (factor alpha 1) with the texture: the texture");
	kinds[1] = _d3d8_ff_texture_cube;
	stages_reset();

	/* point sprites (ClassLib's particles, 0x000a9d20): the texture is on stage 3 (texture *
	diffuse * 2, the diffuse alpha), stages 0-2 have none and stage 1 is off. The XBE's D3D
	builds its one combiner stage from texture stage 3 while D3DRS_POINTSPRITEENABLE is set
	(0x001f8a20); a combiner built from stage 0 drew each particle as a solid square of its
	color (the squares of TankedUp and Special Delivery) */
	{
		static const unsigned char sprite_kinds[4] = { 0, 0, 0, _d3d8_ff_texture_2d };

		in.t[3][0] = 0.25f; in.t[3][1] = 0.0f; in.t[3][2] = 0.5f; in.t[3][3] = 1.0f;
		stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_MODULATE, D3DTA_TEXTURE,
			D3DTA_DIFFUSE);
		stage_state(3, D3DTOP_MODULATE2X, D3DTA_DIFFUSE, D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE,
			D3DTA_TEXTURE);
		D3D__RenderState[D3DRS_POINTSPRITEENABLE] = TRUE;
		run_stages(sprite_kinds, &in, result, &modes);
		CHECK_COLOR(result, 0.25f, 0.0f, 0.5f, 0.3f, "point sprite: stage 3's texture * diffuse * 2");
		CHECK(modes == (1u << 15), "point sprite: texture mode only on stage 3, got 0x%lx", (unsigned long)modes);
		D3D__RenderState[D3DRS_POINTSPRITEENABLE] = FALSE;
		run_stages(sprite_kinds, &in, result, &modes);
		CHECK_COLOR(result, 0.5f, 1.0f, 0.5f, 0.3f, "no point sprite: stage 3 is not reached");
		stages_reset();
	}

	/* specular added, then fog: f * color + (1 - f) * fog color */
	stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_DIFFUSE);
	D3D__RenderState[D3DRS_SPECULARENABLE] = TRUE;
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.5f, 0.25f, 1.0f, 0.8f, "specular added");
	D3D__RenderState[D3DRS_SPECULARENABLE] = FALSE;
	D3D__RenderState[D3DRS_FOGENABLE] = TRUE;
	in.fog[0] = 1.0f; in.fog[1] = 0.0f; in.fog[2] = 0.0f; in.fog[3] = 0.25f;
	run_stages(NULL, &in, result, NULL);
	CHECK_COLOR(result, 0.25f * 0.25f + 0.75f, 0.25f * 0.25f, 0.5f * 0.25f, 0.8f, "fog");
	stages_reset();
}

/* ---------- 3. CPU model of the vertex program decoding (nv2a_vsh.c) */

static unsigned long field(const DWORD *instruction, int word, int low_bit, int bit_count)
{
	return (instruction[word] >> low_bit) & ((1UL << bit_count) - 1);
}

struct vertex_machine
{
	float v[16][4];
	float c[192][4];
	float r[12][4];
	float o[16][4]; /* by output address; o[0] is oPos */
	int a0;
};

static void operand_value(struct vertex_machine *m, const DWORD *in, char which, float out[4])
{
	unsigned long negate, swizzle[4], index, mux;
	const float *source;
	static const float zero[4];
	int i;

	switch (which)
	{
	case 'A':
		negate = field(in, 1, 8, 1);
		swizzle[0] = field(in, 1, 6, 2); swizzle[1] = field(in, 1, 4, 2);
		swizzle[2] = field(in, 1, 2, 2); swizzle[3] = field(in, 1, 0, 2);
		index = field(in, 2, 28, 4); mux = field(in, 2, 26, 2);
		break;
	case 'B':
		negate = field(in, 2, 25, 1);
		swizzle[0] = field(in, 2, 23, 2); swizzle[1] = field(in, 2, 21, 2);
		swizzle[2] = field(in, 2, 19, 2); swizzle[3] = field(in, 2, 17, 2);
		index = field(in, 2, 13, 4); mux = field(in, 2, 11, 2);
		break;
	default:
		negate = field(in, 2, 10, 1);
		swizzle[0] = field(in, 2, 8, 2); swizzle[1] = field(in, 2, 6, 2);
		swizzle[2] = field(in, 2, 4, 2); swizzle[3] = field(in, 2, 2, 2);
		index = (field(in, 2, 0, 2) << 2) | field(in, 3, 30, 2); mux = field(in, 3, 28, 2);
		break;
	}
	switch (mux)
	{
	case 1: source = index == 12 ? m->o[0] : m->r[index]; break;
	case 2: source = m->v[field(in, 1, 9, 4)]; break;
	case 3:
	{
		long constant = (long)field(in, 1, 13, 8);

		if (field(in, 3, 1, 1))
			constant += m->a0;
		if (constant < 0) constant = 0;
		if (constant > 191) constant = 191;
		source = m->c[constant];
		break;
	}
	default: source = zero; break;
	}
	for (i = 0; i < 4; i++)
		out[i] = negate ? -source[swizzle[i]] : source[swizzle[i]];
}

static void write_masked(float *target, const float *value, unsigned long mask)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (mask & (8 >> i))
			target[i] = value[i];
	}
}

static void run_vertex_program(struct vertex_machine *m, const DWORD *program)
{
	unsigned long count = program[0] >> 16, index;
	const DWORD *instructions = program + 1;

	memset(m->r, 0, sizeof(m->r));
	memset(m->o, 0, sizeof(m->o));
	m->o[0][3] = 1.0f;
	m->a0 = 0;
	for (index = 0; index < count; index++)
	{
		const DWORD *in = instructions + index * 4;
		unsigned long mac = field(in, 1, 21, 4), ilu = field(in, 1, 25, 3);
		unsigned long mac_mask = field(in, 3, 24, 4), temporary = field(in, 3, 20, 4);
		unsigned long ilu_mask = field(in, 3, 16, 4), output_mask = field(in, 3, 12, 4);
		unsigned long output_is_register = field(in, 3, 11, 1), output_address = field(in, 3, 3, 8);
		unsigned long from_ilu = field(in, 3, 2, 1);
		float a[4], b[4], c[4], macv[4] = { 0 }, iluv[4] = { 0 };
		int i;

		operand_value(m, in, 'A', a);
		operand_value(m, in, 'B', b);
		operand_value(m, in, 'C', c);
		switch (mac)
		{
		case 1: case 13: memcpy(macv, a, sizeof(macv)); break;
		case 2: for (i = 0; i < 4; i++) macv[i] = a[i] * b[i]; break;
		case 3: for (i = 0; i < 4; i++) macv[i] = a[i] + c[i]; break;
		case 4: for (i = 0; i < 4; i++) macv[i] = a[i] * b[i] + c[i]; break;
		case 5: macv[0] = macv[1] = macv[2] = macv[3] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; break;
		case 6: macv[0] = macv[1] = macv[2] = macv[3] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + b[3]; break;
		case 7: macv[0] = macv[1] = macv[2] = macv[3] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]; break;
		case 8: macv[0] = 1.0f; macv[1] = a[1] * b[1]; macv[2] = a[2]; macv[3] = b[3]; break;
		case 9: for (i = 0; i < 4; i++) macv[i] = fminf(a[i], b[i]); break;
		case 10: for (i = 0; i < 4; i++) macv[i] = fmaxf(a[i], b[i]); break;
		case 11: for (i = 0; i < 4; i++) macv[i] = a[i] < b[i] ? 1.0f : 0.0f; break;
		case 12: for (i = 0; i < 4; i++) macv[i] = a[i] >= b[i] ? 1.0f : 0.0f; break;
		default: break;
		}
		switch (ilu)
		{
		case 1: memcpy(iluv, c, sizeof(iluv)); break;
		case 2: case 3: iluv[0] = iluv[1] = iluv[2] = iluv[3] = 1.0f / c[0]; break;
		case 4: iluv[0] = iluv[1] = iluv[2] = iluv[3] = 1.0f / sqrtf(fabsf(c[0])); break;
		case 5: iluv[0] = exp2f(floorf(c[0])); iluv[1] = c[0] - floorf(c[0]); iluv[2] = exp2f(c[0]); iluv[3] = 1.0f; break;
		default: break;
		}
		if (mac == 13)
			m->a0 = (int)floorf(macv[0] + 0.001f);
		else if (mac && mac_mask)
			write_masked(temporary == 12 ? m->o[0] : m->r[temporary], macv, mac_mask);
		if (ilu && ilu_mask)
		{
			unsigned long t = mac ? 1 : temporary;

			write_masked(t == 12 ? m->o[0] : m->r[t], iluv, ilu_mask);
		}
		if (output_mask && (from_ilu ? ilu : mac) && output_is_register && output_address < 16)
			write_masked(m->o[output_address], from_ilu ? iluv : macv, output_mask);
		if (field(in, 3, 0, 1))
			break;
	}
}

static const DWORD *assemble(const char *text, HRESULT *result, XGBuffer **errors_out)
{
	XGBuffer *constants = NULL, *compiled = NULL, *errors = NULL, *listing = NULL;
	static DWORD copy[4 + 136 * 4 + 4];

	*result = XGAssembleShader("test", text, (UINT)strlen(text), 0, &constants, &compiled, &errors, &listing,
		NULL, NULL, NULL);
	CHECK(constants && errors && listing, "XGAssembleShader returns every buffer the caller asked for");
	if (compiled)
	{
		memcpy(copy, XGBuffer_GetBufferPointer(compiled), XGBuffer_GetBufferSize(compiled) < sizeof(copy) ?
			XGBuffer_GetBufferSize(compiled) : sizeof(copy));
	}
	XGBuffer_Release(constants);
	XGBuffer_Release(compiled);
	XGBuffer_Release(listing);
	if (errors_out)
		*errors_out = errors;
	else
		XGBuffer_Release(errors);
	return *result >= 0 ? copy : NULL;
}

static void set_constant(struct vertex_machine *m, int reg, float x, float y, float z, float w)
{
	float *c = m->c[reg + 96];

	c[0] = x; c[1] = y; c[2] = z; c[3] = w;
}

static void test_assembler(void)
{
	struct vertex_machine m;
	const DWORD *program;
	HRESULT result;
	XGBuffer *errors;
	float clip[4];
	int i;

	memset(&m, 0, sizeof(m));
	/* the viewport constants Direct3D keeps in c[-38] and c[-37] */
	set_constant(&m, -38, 320.0f, -240.0f, 16777215.0f, 0.0f);
	set_constant(&m, -37, 320.0f, 240.0f, 0.0f, 0.0f);
	/* a composite matrix in c[92..95] (rows: dp4 against each) */
	set_constant(&m, 92, 2.0f, 0.0f, 0.0f, 0.5f);
	set_constant(&m, 93, 0.0f, 3.0f, 0.0f, 0.0f);
	set_constant(&m, 94, 0.0f, 0.0f, 0.5f, 0.0f);
	set_constant(&m, 95, 0.0f, 0.0f, 1.0f, 0.0f);
	m.v[0][0] = 1.0f; m.v[0][1] = 2.0f; m.v[0][2] = 4.0f; m.v[0][3] = 1.0f;
	m.v[3][0] = 0.1f; m.v[3][1] = 0.2f; m.v[3][2] = 0.3f; m.v[3][3] = 0.4f;
	m.v[9][0] = 0.6f; m.v[9][1] = 0.7f;

	program = assemble(
		"xvs.1.1\n"
		"; the game's position fragment\n"
		"dp4 oPos.x, v0, c[92]\n"
		"dp4 oPos.y, v0, c[93]\n"
		"dp4 oPos.z, v0, c[94]\n"
		"dp4 oPos.w, v0, c[95]   // w\n"
		"mov oD0, v3\n"
		"mov oT0.xy, v9\n", &result, NULL);
	CHECK(program && result == S_OK, "position shader assembles (0x%lx)", (unsigned long)result);
	if (program)
	{
		CHECK((program[0] & 0xffff) == 0x2078 && (program[0] >> 16) == 8,
			"header: type 0x2078, 6 instructions + 2 of screen conversion, got 0x%08lx", (unsigned long)program[0]);
		run_vertex_program(&m, program);
		clip[0] = 2.0f * 1 + 0.5f; clip[1] = 3.0f * 2; clip[2] = 0.5f * 4; clip[3] = 4.0f;
		for (i = 0; i < 3; i++)
		{
			float expected = clip[i] * m.c[96 - 38][i] / clip[3] + m.c[96 - 37][i];

			CHECK(near(m.o[0][i], expected, fabsf(expected) * 1e-5f + 1e-4f), "oPos[%d] = %f, expected %f", i,
				m.o[0][i], expected);
		}
		CHECK(near(m.o[0][3], clip[3], 1e-5f), "oPos.w keeps the clip w");
		CHECK(near(m.o[3][0], 0.1f, 1e-6f) && near(m.o[3][3], 0.4f, 1e-6f), "oD0 = v3");
		CHECK(near(m.o[9][0], 0.6f, 1e-6f) && near(m.o[9][1], 0.7f, 1e-6f), "oT0.xy = v9");
	}

	/* skinning as the game writes it: a0.x selects the bone's matrix */
	for (i = 0; i < 3; i++)
		set_constant(&m, 10 + i, i == 0 ? 1.0f : 0.0f, i == 1 ? 1.0f : 0.0f, i == 2 ? 1.0f : 0.0f, 5.0f * (i + 1));
	m.v[1][0] = 10.0f;
	program = assemble(
		"xvs.1.1\n"
		"mov a0.x, v1.x\n"
		"dp4 r0.x, v0, c[a0.x + 0]\n"
		"dp4 r0.y, v0, c[a0.x + 1]\n"
		"dp4 r0.z, v0, c[a0.x + 2]\n"
		"mov r0.w, v0.w\n"
		"mov oT1, r0\n", &result, NULL);
	CHECK(program != NULL, "relative addressing assembles");
	if (program)
	{
		run_vertex_program(&m, program);
		CHECK(near(m.o[10][0], 1.0f + 5.0f, 1e-5f) && near(m.o[10][1], 2.0f + 10.0f, 1e-5f) &&
			near(m.o[10][2], 4.0f + 15.0f, 1e-5f) && near(m.o[10][3], 1.0f, 1e-5f),
			"c[a0.x + n]: got (%f %f %f %f)", m.o[10][0], m.o[10][1], m.o[10][2], m.o[10][3]);
		CHECK((program[0] >> 16) == 6, "no screen conversion without a position write");
	}

	/* normalizing (rsq, an ILU operation), swizzles, negation, sub, max */
	m.r[0][0] = 0;
	m.v[2][0] = 3.0f; m.v[2][1] = 0.0f; m.v[2][2] = 4.0f;
	set_constant(&m, 4, 0.0f, 0.0f, -1.0f, 0.5f);
	program = assemble(
		"xvs.1.1\n"
		"mov r2, v2\n"
		"dp3 r11.w, r2, r2\n"
		"rsq r11.w, r11.w\n"
		"mul r2.xyz, r2.xyz, r11.w\n"
		"dp3 r11.w, -c[4], r2\n"
		"max r11.w, r11.w, c[4].x\n"
		"mul oD0, r11.www, c[4].w\n"
		"sub oD1, r2, r2.zzzz\n"
		"rcp oT2.x, c[4].w\n", &result, NULL);
	CHECK(program != NULL, "lighting fragment assembles");
	if (program)
	{
		run_vertex_program(&m, program);
		/* n = (0.6, 0, 0.8); dot(-(0,0,-1), n) = 0.8; * 0.5 */
		CHECK(near(m.o[3][0], 0.4f, 1e-5f) && near(m.o[3][3], 0.4f, 1e-5f), "lit diffuse: got %f", m.o[3][0]);
		CHECK(near(m.o[4][0], 0.6f - 0.8f, 1e-5f) && near(m.o[4][2], 0.0f, 1e-5f), "sub: got %f", m.o[4][0]);
		CHECK(near(m.o[11][0], 2.0f, 1e-5f), "rcp to an output: got %f", m.o[11][0]);
	}

	/* the matrix macro */
	program = assemble("vs.1.1\nm4x4 oPos, v0, c[92]\n", &result, NULL);
	CHECK(program && (program[0] >> 16) == 6, "m4x4 is four dp4 (plus the conversion)");
	if (program)
	{
		run_vertex_program(&m, program);
		CHECK(near(m.o[0][3], 4.0f, 1e-5f) && near(m.o[0][0], 2.5f * 320.0f / 4.0f + 320.0f, 1e-3f),
			"m4x4 position");
	}

	/* a whole program from the game's fragments (pcxbActor.cpp: the
	strings after "xvs.1.1" in the XBE), as baGenerateShader joins them:
	one bone (index v1.x), lit by an ambient color and one distant light,
	fog from the clip w */
	memset(&m, 0, sizeof(m));
	set_constant(&m, -38, 320.0f, -240.0f, 16777215.0f, 0.0f);
	set_constant(&m, -37, 320.0f, 240.0f, 0.0f, 0.0f);
	for (i = 0; i < 4; i++)
		set_constant(&m, 92 + i, i == 0 ? 1.0f : 0.0f, i == 1 ? 1.0f : 0.0f, i == 2 ? 1.0f : 0.0f, i == 3 ? 1.0f : 0.0f);
	for (i = 0; i < 3; i++)
		set_constant(&m, 20 + i, i == 0 ? 1.0f : 0.0f, i == 1 ? 1.0f : 0.0f, i == 2 ? 1.0f : 0.0f, i == 1 ? 2.0f : 0.0f);
	set_constant(&m, 4, 0.1f, 0.2f, 0.3f, 1.0f);   /* ambient */
	set_constant(&m, 5, 0.0f, 0.0f, 1.0f, 0.0f);   /* the light's reversed direction */
	set_constant(&m, 6, 0.5f, 0.5f, 0.5f, 0.0f);   /* its color */
	set_constant(&m, 7, 1.0f, 1.0f, 1.0f, 1.0f);   /* the material */
	m.v[0][0] = 0.25f; m.v[0][1] = 0.5f; m.v[0][2] = 3.0f; m.v[0][3] = 1.0f;
	m.v[1][0] = 20.0f;
	m.v[3][0] = 0.0f; m.v[3][1] = 0.0f; m.v[3][2] = 2.0f;
	m.v[4][0] = 0.125f; m.v[4][1] = 0.875f;
	program = assemble(
		"xvs.1.1\n"
		"mov a0.x, v1.x\n"
		"dp4 r0.x, v0, c[a0.x + 0]\n"
		"dp4 r0.y, v0, c[a0.x + 1]\n"
		"dp4 r0.z, v0, c[a0.x + 2]\n"
		"mov r0.w, v0.w\n"
		"dp3 r2.x, v3, c[a0.x + 0]\n"
		"dp3 r2.y, v3, c[a0.x + 1]\n"
		"dp3 r2.z, v3, c[a0.x + 2]\n"
		"mov r2.w, v0.w\n"
		"dp3 r11.w, r2, r2\n"
		"rsq r11.w, r11.w\n"
		"mul r2.xyz, r2.xyz, r11.w\n"
		"mov r3, c[4]\n"
		"dp3 r11.w, c[5], r2\n"
		"max r11.w, c[6].w, r11.w\n"
		"mad r3.xyz, c[6].xyz, r11.www, r3.xyz\n"
		"dp4 oPos.x, r0, c[92]\n"
		"dp4 oPos.y, r0, c[93]\n"
		"dp4 oPos.z, r0, c[94]\n"
		"dp4 oPos.w, r0, c[95]\n"
		"dp4 oFog.x, r0, c[95]\n"
		"mul oD0, r3, c[7]\n"
		"mov oT0, v4\n", &result, NULL);
	CHECK(program != NULL, "the game's fragments assemble");
	if (program)
	{
		run_vertex_program(&m, program);
		/* the bone moves y by 2: (0.25, 2.5, 3, 1); the normal (0, 0, 1)
		faces the light fully */
		CHECK(near(m.o[0][0], 0.25f * 320.0f + 320.0f, 1e-3f) && near(m.o[0][1], 2.5f * -240.0f + 240.0f, 1e-3f) &&
			near(m.o[0][3], 1.0f, 1e-6f), "skinned position: (%f %f %f %f)", m.o[0][0], m.o[0][1], m.o[0][2], m.o[0][3]);
		CHECK(near(m.o[3][0], 0.6f, 1e-5f) && near(m.o[3][1], 0.7f, 1e-5f) && near(m.o[3][2], 0.8f, 1e-5f) &&
			near(m.o[3][3], 1.0f, 1e-5f), "lit color: (%f %f %f %f)", m.o[3][0], m.o[3][1], m.o[3][2], m.o[3][3]);
		CHECK(near(m.o[5][0], 1.0f, 1e-6f), "oFog.x = clip w");
		CHECK(near(m.o[9][1], 0.875f, 1e-6f), "oT0 = v4");
	}

	/* failures: a message, E_FAIL, and the buffers still there */
	program = assemble("xvs.1.1\nfrob r0, v0\n", &result, &errors);
	CHECK(!program && result < 0, "an unknown instruction fails");
	CHECK(errors && strstr((const char *)XGBuffer_GetBufferPointer(errors), "unknown instruction"),
		"the error log names the problem");
	XGBuffer_Release(errors);
	program = assemble("xvs.1.1\nmad r0, c[1], v0, c[2]\n", &result, NULL);
	CHECK(!program, "two constants in one instruction are refused");
	program = assemble("xvs.1.1\nmov r0, v0\nmov r1, q5\n", &result, &errors);
	CHECK(!program && strstr((const char *)XGBuffer_GetBufferPointer(errors), "(3)"),
		"errors carry the line number");
	XGBuffer_Release(errors);
	CHECK(XGBuffer_Release(NULL) == 0, "releasing nothing is harmless");
}

/* ---------- 4. XGSetTextureHeader */

static void test_texture_header(void)
{
	D3DTexture texture;
	struct xgpu_texture_description description;

	memset(&texture, 0xcc, sizeof(texture));
	/* as the game does for its screen copy (0x00044e90) */
	XGSetTextureHeader(640, 480, 1, 0, D3DFMT_LIN_X8R8G8B8, 0, &texture, 0, 640 * 4);
	CHECK(texture.Common == 0x40001 && texture.Data == 0 && texture.Lock == 0, "common 0x%lx data 0x%lx",
		(unsigned long)texture.Common, (unsigned long)texture.Data);
	xgpu_texture_describe(texture.Format, texture.Size, &description);
	CHECK(description.format == D3DFMT_LIN_X8R8G8B8 && description.linear && description.width == 640 &&
		description.height == 480 && description.pitch == 2560 && description.levels == 1,
		"linear header: format %lu %lux%lu pitch %lu levels %lu", (unsigned long)description.format,
		description.width, description.height, description.pitch, description.levels);
	CHECK(texture.Format & D3DFORMAT_BORDERSOURCE_COLOR, "the border color is the default border");
	XGSetTextureHeader(256, 128, 3, D3DUSAGE_BORDERSOURCE_TEXTURE, D3DFMT_A8R8G8B8, 0, &texture, 0x1000, 0);
	xgpu_texture_describe(texture.Format, texture.Size, &description);
	CHECK(!description.linear && description.width == 256 && description.height == 128 && description.levels == 3 &&
		texture.Data == 0x1000 && !(texture.Format & D3DFORMAT_BORDERSOURCE_COLOR),
		"swizzled header: %lux%lu levels %lu", description.width, description.height, description.levels);
	/* the library (0x002283c9): levels 0 on a swizzled texture is the whole
	chain, max(log2 width, log2 height) + 1; a level count of 1 would leave
	the texture's smaller levels unread */
	XGSetTextureHeader(256, 64, 0, 0, D3DFMT_A8R8G8B8, 0, &texture, 0, 0);
	xgpu_texture_describe(texture.Format, texture.Size, &description);
	CHECK(description.levels == 9, "levels 0 is the whole chain: %lu", description.levels);
	/* the library: a linear texture's default pitch is the row rounded up
	to 64 bytes (100 * 4 = 400 -> 448), not the bare row */
	XGSetTextureHeader(100, 10, 1, 0, D3DFMT_LIN_X8R8G8B8, 0, &texture, 0, 0);
	xgpu_texture_describe(texture.Format, texture.Size, &description);
	CHECK(description.pitch == 448 && description.width == 100, "default linear pitch %lu", description.pitch);
}

/* ---------- GLSL dumps (OPENFRENZY_D3D8_TEST_GLSL: a directory)

run_tests.sh compiles each with glslangValidator: the generated shaders are
otherwise only compiled by a GL driver, which the headless run has not */

static unsigned long glsl_dumps;

static void dump_glsl(const char *directory, const char *name, const char *suffix, char *source)
{
	char path[1024];
	FILE *file;

	snprintf(path, sizeof(path), "%s/%s.%s", directory, name, suffix);
	file = fopen(path, "w");
	CHECK(file != NULL, "cannot write %s", path);
	if (file)
	{
		fputs(source, file);
		fclose(file);
		glsl_dumps++;
	}
	free(source);
}

static void dump_vertex(const char *directory, const char *name, DWORD fvf)
{
	struct d3d8_ff_vertex_key key;

	d3d8_ff_vertex_key_build(fvf, &key);
	dump_glsl(directory, name, "vert", d3d8_ff_vertex_glsl(&key));
}

static void dump_pixel(const char *directory, const char *name, const unsigned char *kinds)
{
	struct nv2a_pixel_shader_key key;
	int stage;

	memset(&key, 0, sizeof(key));
	d3d8_ff_combiners(kinds, key.combiner_state, &key.texture_modes);
	for (stage = 0; stage < 4; stage++)
		key.sampler_type[stage] = kinds[stage] == _d3d8_ff_texture_cube ? _xgpu_sampler_cube :
			kinds[stage] == _d3d8_ff_texture_3d ? _xgpu_sampler_3d : kinds[stage] ? _xgpu_sampler_2d : 0;
	key.fog_enable = D3D__RenderState[D3DRS_FOGENABLE] != 0;
	key.fog_table_mode = (unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
	key.alpha_test_function = D3D__RenderState[D3DRS_ALPHATESTENABLE] ? D3DCMP_GREATER : 0;
	dump_glsl(directory, name, "frag", nv2a_pixel_shader_to_glsl(&key));
}

static void test_glsl_dumps(void)
{
	const char *directory = getenv("OPENFRENZY_D3D8_TEST_GLSL");
	static const unsigned char one_2d[4] = { 1, 0, 0, 0 };
	static const unsigned char two[4] = { 1, 3, 0, 0 };
	static const unsigned char none[4] = { 0, 0, 0, 0 };
	D3DLIGHT8 light;
	int index;

	if (!directory || !*directory)
	{
		printf("d3d8_test: GLSL dumps skipped (OPENFRENZY_D3D8_TEST_GLSL not set)\n");
		return;
	}
	xgpu_capabilities.shading_language = "300 es";
	memset(D3D__RenderState, 0, sizeof(D3D__RenderState));
	stages_reset();
	/* unlit vertices of each position kind */
	dump_vertex(directory, "ff_rhw_diffuse_tex1", D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	dump_vertex(directory, "ff_xyz", D3DFVF_XYZ);
	dump_vertex(directory, "ff_xyz_diffuse_tex1", D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	/* lit, with every light type, specular, a local viewer, normalizing,
	material sources, range fog, texture generation and projected
	transforms */
	D3D__RenderState[D3DRS_LIGHTING] = TRUE;
	D3D__RenderState[D3DRS_SPECULARENABLE] = TRUE;
	D3D__RenderState[D3DRS_LOCALVIEWER] = TRUE;
	D3D__RenderState[D3DRS_NORMALIZENORMALS] = TRUE;
	D3D__RenderState[D3DRS_COLORVERTEX] = TRUE;
	D3D__RenderState[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1;
	D3D__RenderState[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
	D3D__RenderState[D3DRS_FOGENABLE] = TRUE;
	D3D__RenderState[D3DRS_FOGTABLEMODE] = D3DFOG_LINEAR;
	D3D__RenderState[D3DRS_RANGEFOGENABLE] = TRUE;
	for (index = 0; index < 3; index++)
	{
		memset(&light, 0, sizeof(light));
		light.Type = (D3DLIGHTTYPE)(index + 1);
		light.Range = 100.0f;
		light.Attenuation0 = 1.0f;
		D3DDevice_SetLight(10 + index, &light);
		D3DDevice_LightEnable(10 + index, TRUE);
	}
	D3D__TextureState[0][D3DTSS_TEXCOORDINDEX] = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;
	D3D__TextureState[0][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_COUNT3 | D3DTTFF_PROJECTED;
	D3D__TextureState[1][D3DTSS_TEXCOORDINDEX] = 0;
	D3D__TextureState[1][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_COUNT2;
	D3D__TextureState[2][D3DTSS_TEXCOORDINDEX] = D3DTSS_TCI_CAMERASPACENORMAL;
	D3D__TextureState[3][D3DTSS_TEXCOORDINDEX] = D3DTSS_TCI_CAMERASPACEPOSITION;
	D3D__TextureState[3][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_COUNT4 | D3DTTFF_PROJECTED;
	dump_vertex(directory, "ff_lit_all", 0x152 | D3DFVF_SPECULAR);
	D3D__RenderState[D3DRS_FOGTABLEMODE] = D3DFOG_NONE;
	D3D__RenderState[D3DRS_LOCALVIEWER] = FALSE;
	dump_vertex(directory, "ff_lit_vertex_fog", D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(1));
	dump_vertex(directory, "ff_lit_no_normal", D3DFVF_XYZ | D3DFVF_DIFFUSE);
	for (index = 0; index < 3; index++)
		D3DDevice_LightEnable(10 + index, FALSE);

	/* pixel shaders from stage states: each operation once, fog, alpha
	test, a cube map stage, and the default stage without a texture */
	stages_reset();
	D3D__RenderState[D3DRS_FOGENABLE] = FALSE;
	for (index = D3DTOP_SELECTARG1; index <= D3DTOP_BUMPENVMAPLUMINANCE; index++)
	{
		char name[32];

		stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_DIFFUSE);
		stage_state(1, (DWORD)index, D3DTA_TEXTURE | D3DTA_COMPLEMENT, D3DTA_CURRENT | D3DTA_ALPHAREPLICATE,
			(DWORD)(index == D3DTOP_DOTPRODUCT3 ? D3DTOP_SELECTARG1 : index), D3DTA_TFACTOR, D3DTA_SPECULAR);
		D3D__TextureState[1][D3DTSS_COLORARG0] = D3DTA_TEMP;
		D3D__TextureState[1][D3DTSS_ALPHAARG0] = D3DTA_DIFFUSE;
		snprintf(name, sizeof(name), "ps_op%02d", index);
		dump_pixel(directory, name, two);
	}
	stages_reset();
	stage_state(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT);
	dump_pixel(directory, "ps_default_no_texture", none);
	D3D__RenderState[D3DRS_FOGENABLE] = TRUE;
	D3D__RenderState[D3DRS_FOGTABLEMODE] = D3DFOG_EXP2;
	D3D__RenderState[D3DRS_SPECULARENABLE] = TRUE;
	D3D__RenderState[D3DRS_ALPHATESTENABLE] = TRUE;
	dump_pixel(directory, "ps_fog_specular_alphatest", one_2d);
	stages_reset();
	memset(D3D__RenderState, 0, sizeof(D3D__RenderState));
	printf("d3d8_test: %lu GLSL files written to %s\n", glsl_dumps, directory);
}

/* ---------- 5. drawing (needs a window) */

struct test_vertex
{
	float x, y, z, rhw;
	D3DCOLOR color;
	float u, v;
};

struct lit_vertex
{
	float x, y, z;
	float nx, ny, nz;
	D3DCOLOR color;
	float u, v;
};

/* a pixel of the 640x480 back buffer, at Direct3D coordinates: the GL
target may hold more pixels (the device draws at the window's resolution),
so the point goes through the draw's GL viewport (the whole target); rows
count from the top in both (the GLES shaders flip y) */
static void read_pixel(int x, int y, unsigned char out[4])
{
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	memset(out, 0, 4);
	glReadPixels(viewport[0] + (GLint)((x + 0.5f) * viewport[2] / 640.0f),
		viewport[1] + (GLint)((y + 0.5f) * viewport[3] / 480.0f), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out);
}

static int pixel_near(const unsigned char *pixel, int r, int g, int b)
{
	return abs(pixel[0] - r) <= 3 && abs(pixel[1] - g) <= 3 && abs(pixel[2] - b) <= 3;
}

static void test_drawing(void)
{
	const char *driver = getenv("SDL_VIDEO_DRIVER");
	D3DPRESENT_PARAMETERS parameters;
	D3DDevice *device;
	D3DCAPS8 caps;
	D3DFIELD_STATUS field;
	struct test_vertex quad[4];
	struct lit_vertex lit[4];
	D3DMATERIAL8 material;
	D3DLIGHT8 light;
	D3DMATRIX identity;
	unsigned char pixel[4];
	int i;

	CHECK(Direct3DCreate8(0) != NULL, "Direct3DCreate8 (0x002022e0) returns non-NULL");
	CHECK(Direct3D_GetDeviceCaps(0, D3DDEVTYPE_HAL, &caps) == S_OK && caps.MaxActiveLights == 8,
		"Direct3D_GetDeviceCaps");
	CHECK(Direct3D_GetDeviceCaps(1, D3DDEVTYPE_HAL, &caps) < 0, "adapter 1 does not exist");
	if (driver && !strcmp(driver, "dummy"))
	{
		printf("d3d8_test: drawing skipped (SDL_VIDEO_DRIVER=dummy)\n");
		return;
	}
	memset(&parameters, 0, sizeof(parameters));
	parameters.BackBufferWidth = 640;
	parameters.BackBufferHeight = 480;
	if (Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &parameters, &device) < 0)
	{
		CHECK(0, "Direct3D_CreateDevice");
		return;
	}
	D3DDevice_GetDisplayFieldStatus(&field);
	CHECK(field.Field == D3DFIELD_PROGRESSIVE, "progressive display");
	/* (the device made the window and context, or found it cannot) */
	if (!platform_video_initialize(640, 480))
	{
		printf("d3d8_test: drawing skipped (no window or GL context)\n");
		return;
	}

	/* an unlit, untextured quad: the default stage 0 gives the vertex color */
	D3DDevice_Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
	D3DDevice_SetRenderState_ZEnable(FALSE);
	for (i = 0; i < 4; i++)
	{
		quad[i].x = (i & 1) ? 300.0f : 100.0f;
		quad[i].y = (i & 2) ? 300.0f : 100.0f;
		quad[i].z = 0.5f;
		quad[i].rhw = 1.0f;
		quad[i].color = 0xffff8000;
		quad[i].u = quad[i].v = 0.0f;
	}
	D3DDevice_SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, quad, sizeof(quad[0]));
	read_pixel(200, 200, pixel);
	CHECK(pixel_near(pixel, 255, 128, 0), "XYZRHW quad drawn in its vertex color: got %d %d %d", pixel[0], pixel[1],
		pixel[2]);
	read_pixel(50, 50, pixel);
	CHECK(pixel_near(pixel, 0, 0, 255), "outside the quad: the clear color, got %d %d %d", pixel[0], pixel[1],
		pixel[2]);

	/* a texture factor stage: modulate by half grey */
	D3DDevice_SetRenderState_TextureFactor(0xff808080);
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, quad, sizeof(quad[0]));
	read_pixel(200, 200, pixel);
	CHECK(pixel_near(pixel, 128, 64, 0), "TFACTOR * diffuse: got %d %d %d", pixel[0], pixel[1], pixel[2]);

	/* a lit quad (XYZ | NORMAL | DIFFUSE | TEX1, 0x152) with identity
	transforms and an orthographic-like projection: a white directional
	light at 60 degrees gives half the material's diffuse */
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	D3DDevice_SetTextureState_Deferred(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	memset(&identity, 0, sizeof(identity));
	identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
	D3DDevice_SetTransform(D3DTS_WORLD, &identity);
	D3DDevice_SetTransform(D3DTS_VIEW, &identity);
	D3DDevice_SetTransform(D3DTS_PROJECTION, &identity);
	for (i = 0; i < 4; i++)
	{
		lit[i].x = (i & 1) ? 0.5f : -0.5f;
		lit[i].y = (i & 2) ? -0.5f : 0.5f;
		lit[i].z = 0.5f;
		lit[i].nx = 0.0f; lit[i].ny = 0.0f; lit[i].nz = -1.0f;
		lit[i].color = 0xff00ff00;
		lit[i].u = lit[i].v = 0.0f;
	}
	memset(&material, 0, sizeof(material));
	material.Diffuse.r = 1.0f; material.Diffuse.g = 1.0f; material.Diffuse.b = 1.0f; material.Diffuse.a = 1.0f;
	material.Ambient.r = 0.25f;
	D3DDevice_SetMaterial(&material);
	memset(&light, 0, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = 1.0f;
	light.Direction.x = 0.0f; light.Direction.y = sqrtf(3.0f) / 2.0f; light.Direction.z = 0.5f;
	CHECK(D3DDevice_SetLight(0, &light) == S_OK, "SetLight");
	CHECK(D3DDevice_LightEnable(0, TRUE) == S_OK, "LightEnable");
	D3DDevice_SetRenderState_Deferred(D3DRS_LIGHTING, TRUE);
	D3DDevice_SetRenderState_Deferred(D3DRS_AMBIENT, 0xff404040);
	/* the vertex color is the diffuse material (COLORVERTEX, COLOR1) */
	D3DDevice_SetVertexShader(0x152);
	D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, lit, sizeof(lit[0]));
	read_pixel(320, 240, pixel);
	/* green 0.5 from the light; red 0.25 * 0.25 from the ambient light */
	CHECK(pixel_near(pixel, 16, 128, 0), "lit quad: got %d %d %d", pixel[0], pixel[1], pixel[2]);
	D3DDevice_SetRenderState_Deferred(D3DRS_COLORVERTEX, FALSE);
	D3DDevice_DrawVerticesUP(D3DPT_TRIANGLESTRIP, 4, lit, sizeof(lit[0]));
	read_pixel(320, 240, pixel);
	CHECK(pixel_near(pixel, 128 + 16, 128, 128), "COLORVERTEX off: the material's white, got %d %d %d", pixel[0],
		pixel[1], pixel[2]);
	D3D_KickOffAndWaitForIdle();
	CHECK(D3DDevice_InsertFence() != D3DDevice_InsertFence(), "fences count up");
	D3DDevice_Present(NULL, NULL, NULL, NULL);
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("d3d8_test: started\n");
	test_fvf();
	test_ff_uniforms();
	test_combiners();
	test_assembler();
	test_texture_header();
	test_glsl_dumps();
	test_drawing();
	printf("d3d8_test: %d checks, %d failed\n", checks, failures);
	return failures > 255 ? 255 : failures;
}
