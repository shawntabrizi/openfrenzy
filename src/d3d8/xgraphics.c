/*
XGRAPHICS.C

The XGRAPHICS (XGRPH section) functions the game calls:

- XGBuffer_Release (0x0022184b), XGBuffer_GetBufferPointer (0x00221874);
- XGAssembleShader (0x0022835a): the game writes its actors' vertex
  shaders as text at run time (bActor's baGenerateShader, 0x0003ae20) and
  assembles them here. The text is vs.1.1 / xvs.1.1; the result is NV2A
  vertex program microcode, encoded as nv2a_vsh.c decodes it, so that
  D3DDevice_CreateVertexShader takes it as it takes any compiled shader;
- XGSetTextureHeader (0x002285c1).

New for openfrenzy. The microcode field layout is the one nv2a_vsh.c reads
(from OpenCE); the instruction set and the screen-space epilogue the XDK's
assembler appends are public Xbox facts (nxdk, xemu).
*/

#include "xgpu.h"
#include "d3d8_extra.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- XGBuffer */

static XGBuffer *buffer_create(const void *data, unsigned long size)
{
	XGBuffer *buffer = calloc(1, sizeof(*buffer));

	if (!buffer)
		return NULL;
	/* (one byte more, so that text is terminated) */
	buffer->pData = calloc(1, size + 1);
	if (!buffer->pData)
	{
		free(buffer);
		return NULL;
	}
	if (data && size)
		memcpy(buffer->pData, data, size);
	buffer->size = (DWORD)size;
	buffer->refCount = 1;
	return buffer;
}

/* 0x0022184b */
ULONG WINAPI XGBuffer_Release(XGBuffer *buffer)
{
	ULONG count;

	if (!buffer)
		return 0;
	count = --buffer->refCount;
	if (!count)
	{
		free(buffer->pData);
		free(buffer);
	}
	return count;
}

/* 0x00221874 */
void *WINAPI XGBuffer_GetBufferPointer(XGBuffer *buffer)
{
	return buffer ? buffer->pData : NULL;
}

DWORD WINAPI XGBuffer_GetBufferSize(XGBuffer *buffer)
{
	return buffer ? buffer->size : 0;
}

/* ---------- XGSetTextureHeader */

static unsigned long log2_floor(unsigned long value)
{
	unsigned long result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}
	return result;
}

/* 0x002285c1: as the library's (0x00228573): a texture with one reference,
its data at `data` (a physical address, or 0 until D3DResource_Register),
linear formats sized by width, height and pitch, swizzled ones by their
logarithms */
VOID WINAPI XGSetTextureHeader(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
	D3DPOOL pool, D3DTexture *texture, UINT data, UINT pitch)
{
	struct xgpu_texture_description description;
	DWORD format_word;

	(void)pool;
	if (!texture)
		return;
	if (!width)
		width = 1;
	if (!height)
		height = 1;
	xgpu_texture_describe(((DWORD)format << D3DFORMAT_FORMAT_SHIFT) | (1 << D3DFORMAT_MIPMAP_SHIFT), 0, &description);
	format_word = ((DWORD)format << D3DFORMAT_FORMAT_SHIFT) | (2 << D3DFORMAT_DIMENSION_SHIFT) |
		D3DFORMAT_DMACHANNEL_A;
	if (!(usage & D3DUSAGE_BORDERSOURCE_TEXTURE))
		format_word |= D3DFORMAT_BORDERSOURCE_COLOR;
	texture->Common = D3DCOMMON_TYPE_TEXTURE | 1;
	texture->Data = data;
	texture->Lock = 0;
	if (description.linear)
	{
		/* (the library: levels 0 is 1; the default pitch is the row
		rounded up to 64 bytes) */
		if (!levels)
			levels = 1;
		if (!pitch)
			pitch = (width * (UINT)description.pitch + D3DTEXTURE_PITCH_ALIGNMENT - 1) &
				~(UINT)(D3DTEXTURE_PITCH_ALIGNMENT - 1);
		texture->Format = format_word | ((DWORD)levels << D3DFORMAT_MIPMAP_SHIFT);
		texture->Size = (((pitch / D3DTEXTURE_PITCH_ALIGNMENT) - 1) << D3DSIZE_PITCH_SHIFT) |
			((height - 1) << D3DSIZE_HEIGHT_SHIFT) | (width - 1);
	}
	else
	{
		/* (the library: levels 0 is the whole chain, down to 1 by 1) */
		if (!levels)
			levels = (UINT)(log2_floor(width > height ? width : height) + 1);
		texture->Format = format_word | ((DWORD)levels << D3DFORMAT_MIPMAP_SHIFT) |
			(log2_floor(width) << D3DFORMAT_USIZE_SHIFT) | (log2_floor(height) << D3DFORMAT_VSIZE_SHIFT);
		texture->Size = 0;
	}
}

/* ---------- the vertex shader assembler */

enum
{
	_mac_nop, _mac_mov, _mac_mul, _mac_add, _mac_mad, _mac_dp3, _mac_dph, _mac_dp4,
	_mac_dst, _mac_min, _mac_max, _mac_slt, _mac_sge, _mac_arl,
};

enum
{
	_ilu_nop, _ilu_mov, _ilu_rcp, _ilu_rcc, _ilu_rsq, _ilu_exp, _ilu_log, _ilu_lit,
};

enum
{
	_mux_none, _mux_temporary, _mux_input, _mux_constant,
};

/* output register addresses */
enum
{
	_out_pos = 0, _out_d0 = 3, _out_d1 = 4, _out_fog = 5, _out_pts = 6, _out_b0 = 7, _out_b1 = 8, _out_t0 = 9,
};

#define MAX_INSTRUCTIONS 136
/* the hardware number of D3D constant c[0] */
#define CONSTANT_BIAS 96
#define POSITION_TEMPORARY 12

struct source
{
	int mux;
	int index;      /* temporary, input or hardware constant number */
	int relative;   /* c[a0.x + n] */
	int negate;
	int swizzle[4];
};

struct destination
{
	enum { _dest_temporary, _dest_output, _dest_address, _dest_constant } kind;
	int index;
	int mask;       /* bit 3 is x */
};

struct assembler
{
	const char *text;
	unsigned long length;
	unsigned long line;
	DWORD instructions[MAX_INSTRUCTIONS][4];
	unsigned long count;
	BOOL screen_space;   /* vs and xvs: the viewport conversion is appended */
	BOOL writes_position;
	DWORD type;
	char errors[2048];
	unsigned long error_length;
	BOOL failed;
	DWORD constants[1024];
	unsigned long constant_words;
};

static void assembler_error(struct assembler *as, const char *format, ...)
{
	va_list arguments;
	int written;

	as->failed = TRUE;
	if (as->error_length >= sizeof(as->errors) - 1)
		return;
	written = snprintf(as->errors + as->error_length, sizeof(as->errors) - as->error_length, "(%lu): error: ",
		as->line);
	if (written > 0)
		as->error_length += (unsigned long)written;
	if (as->error_length >= sizeof(as->errors) - 1)
		return;
	va_start(arguments, format);
	written = vsnprintf(as->errors + as->error_length, sizeof(as->errors) - as->error_length, format, arguments);
	va_end(arguments);
	if (written > 0)
		as->error_length += (unsigned long)written;
	if (as->error_length < sizeof(as->errors) - 1)
		as->errors[as->error_length++] = '\n';
	if (as->error_length >= sizeof(as->errors))
		as->error_length = sizeof(as->errors) - 1;
	as->errors[as->error_length] = 0;
}

static const char *skip_spaces(const char *p)
{
	while (*p == ' ' || *p == '\t' || *p == '\r')
		p++;
	return p;
}

static int swizzle_component(char c)
{
	switch (tolower((unsigned char)c))
	{
	case 'x': case 'r': return 0;
	case 'y': case 'g': return 1;
	case 'z': case 'b': return 2;
	case 'w': case 'a': return 3;
	default: return -1;
	}
}

static BOOL parse_number(const char **p, long *value)
{
	char *end;
	const char *q = skip_spaces(*p);

	*value = strtol(q, &end, 10);
	if (end == q)
		return FALSE;
	*p = end;
	return TRUE;
}

/* c[n], c[a0.x + n], c[n + a0.x], c[a0.x], c[a0.x - n], cN */
static BOOL parse_constant(struct assembler *as, const char **p, int *index, int *relative)
{
	const char *q = *p + 1;
	long value = 0, number;
	BOOL have_number = FALSE;

	*relative = 0;
	q = skip_spaces(q);
	if (*q != '[')
	{
		if (!parse_number(&q, &value))
		{
			assembler_error(as, "bad constant register");
			return FALSE;
		}
		*index = (int)value + CONSTANT_BIAS;
		*p = q;
		return TRUE;
	}
	q++;
	for (;;)
	{
		int sign = 1;

		q = skip_spaces(q);
		if (*q == ']')
			break;
		if (*q == '+')
		{
			q++;
			continue;
		}
		if (*q == '-')
		{
			sign = -1;
			q = skip_spaces(q + 1);
		}
		if (!strncasecmp(q, "a0.x", 4))
		{
			*relative = 1;
			q += 4;
			continue;
		}
		if (!strncasecmp(q, "a0", 2))
		{
			*relative = 1;
			q += 2;
			continue;
		}
		if (!parse_number(&q, &number))
		{
			assembler_error(as, "bad constant register index");
			return FALSE;
		}
		value += sign * number;
		have_number = TRUE;
	}
	(void)have_number;
	*p = q + 1;
	*index = (int)value + CONSTANT_BIAS;
	if (*index < 0 || *index >= XGPU_VERTEX_CONSTANT_COUNT)
	{
		assembler_error(as, "constant register c[%ld] out of range", value);
		return FALSE;
	}
	return TRUE;
}

static BOOL parse_source(struct assembler *as, const char *text, struct source *source)
{
	const char *p = skip_spaces(text);
	long number;
	int count = 0, component;

	memset(source, 0, sizeof(*source));
	if (*p == '-')
	{
		source->negate = 1;
		p = skip_spaces(p + 1);
	}
	switch (tolower((unsigned char)*p))
	{
	case 'r':
		p++;
		if (!parse_number(&p, &number) || number < 0 || number > POSITION_TEMPORARY)
		{
			assembler_error(as, "bad temporary register in '%s'", text);
			return FALSE;
		}
		source->mux = _mux_temporary;
		source->index = (int)number;
		break;
	case 'v':
		p++;
		if (!parse_number(&p, &number) || number < 0 || number > 15)
		{
			assembler_error(as, "bad input register in '%s'", text);
			return FALSE;
		}
		source->mux = _mux_input;
		source->index = (int)number;
		break;
	case 'c':
		if (!parse_constant(as, &p, &source->index, &source->relative))
			return FALSE;
		source->mux = _mux_constant;
		break;
	case 'o':
		if (!strncasecmp(p, "opos", 4))
		{
			/* the position, as written so far (r12) */
			p += 4;
			source->mux = _mux_temporary;
			source->index = POSITION_TEMPORARY;
			break;
		}
		/* fall through */
	default:
		assembler_error(as, "bad source register '%s'", text);
		return FALSE;
	}
	p = skip_spaces(p);
	if (*p == '.')
	{
		p++;
		while ((component = swizzle_component(*p)) >= 0 && count < 4)
		{
			source->swizzle[count++] = component;
			p++;
		}
		if (!count)
		{
			assembler_error(as, "bad swizzle in '%s'", text);
			return FALSE;
		}
	}
	if (!count)
	{
		for (count = 0; count < 4; count++)
			source->swizzle[count] = count;
	}
	/* a short swizzle repeats its last component */
	for (; count < 4; count++)
		source->swizzle[count] = source->swizzle[count - 1];
	p = skip_spaces(p);
	if (*p)
	{
		assembler_error(as, "unexpected '%s' in source '%s'", p, text);
		return FALSE;
	}
	return TRUE;
}

static BOOL parse_destination(struct assembler *as, const char *text, struct destination *destination)
{
	const char *p = skip_spaces(text);
	long number;
	int component;

	memset(destination, 0, sizeof(*destination));
	if (!strncasecmp(p, "a0", 2))
	{
		destination->kind = _dest_address;
		p += 2;
	}
	else if (tolower((unsigned char)*p) == 'r')
	{
		p++;
		if (!parse_number(&p, &number) || number < 0 || number > POSITION_TEMPORARY)
		{
			assembler_error(as, "bad temporary register '%s'", text);
			return FALSE;
		}
		destination->kind = _dest_temporary;
		destination->index = (int)number;
		if (number == POSITION_TEMPORARY)
			as->writes_position = TRUE;
	}
	else if (tolower((unsigned char)*p) == 'c')
	{
		int relative;

		if (!parse_constant(as, &p, &destination->index, &relative))
			return FALSE;
		if (relative)
		{
			assembler_error(as, "relative constant destinations are not supported");
			return FALSE;
		}
		destination->kind = _dest_constant;
	}
	else if (tolower((unsigned char)*p) == 'o')
	{
		static const struct { const char *name; int address; } outputs[] =
		{
			{ "opos", _out_pos }, { "od0", _out_d0 }, { "od1", _out_d1 }, { "ofog", _out_fog }, { "opts", _out_pts },
			{ "ob0", _out_b0 }, { "ob1", _out_b1 }, { "ot0", _out_t0 }, { "ot1", _out_t0 + 1 }, { "ot2", _out_t0 + 2 },
			{ "ot3", _out_t0 + 3 },
		};
		unsigned long index;

		for (index = 0; index < sizeof(outputs) / sizeof(outputs[0]); index++)
		{
			unsigned long length = strlen(outputs[index].name);

			if (!strncasecmp(p, outputs[index].name, length) && !isalnum((unsigned char)p[length]))
				break;
		}
		if (index == sizeof(outputs) / sizeof(outputs[0]))
		{
			assembler_error(as, "bad output register '%s'", text);
			return FALSE;
		}
		p += strlen(outputs[index].name);
		if (outputs[index].address == _out_pos && as->screen_space)
		{
			/* the program's position goes to r12; the conversion to the
			screen, appended at the end, writes oPos from it */
			destination->kind = _dest_temporary;
			destination->index = POSITION_TEMPORARY;
			as->writes_position = TRUE;
		}
		else
		{
			destination->kind = _dest_output;
			destination->index = outputs[index].address;
		}
	}
	else
	{
		assembler_error(as, "bad destination register '%s'", text);
		return FALSE;
	}
	p = skip_spaces(p);
	if (*p == '.')
	{
		p++;
		while ((component = swizzle_component(*p)) >= 0)
		{
			destination->mask |= 8 >> component;
			p++;
		}
		if (!destination->mask)
		{
			assembler_error(as, "bad write mask in '%s'", text);
			return FALSE;
		}
	}
	else
	{
		destination->mask = 0xf;
	}
	p = skip_spaces(p);
	if (*p)
	{
		assembler_error(as, "unexpected '%s' in destination '%s'", p, text);
		return FALSE;
	}
	return TRUE;
}

static DWORD swizzle_bits(const struct source *source)
{
	return (DWORD)((source->swizzle[0] << 6) | (source->swizzle[1] << 4) | (source->swizzle[2] << 2) |
		source->swizzle[3]);
}

/* one instruction: a MAC operation, or an ILU one, on operands A, B and C */
static void emit(struct assembler *as, int mac, int ilu, const struct source *a, const struct source *b,
	const struct source *c, const struct destination *destination)
{
	static const struct source unused = { _mux_temporary, 0, 0, 0, { 0, 1, 2, 3 } };
	const struct source *operands[3];
	DWORD *words;
	int input = 0, constant = 0, relative = 0, have_input = 0, have_constant = 0, index;

	if (as->count >= MAX_INSTRUCTIONS)
	{
		if (as->count == MAX_INSTRUCTIONS)
			assembler_error(as, "more than %d instructions", MAX_INSTRUCTIONS);
		as->count++;
		return;
	}
	operands[0] = a ? a : &unused;
	operands[1] = b ? b : &unused;
	operands[2] = c ? c : &unused;
	/* one input register and one constant for the whole instruction */
	for (index = 0; index < 3; index++)
	{
		const struct source *operand = operands[index];

		if (operand->mux == _mux_input)
		{
			if (have_input && input != operand->index)
				assembler_error(as, "two different input registers in one instruction");
			input = operand->index;
			have_input = 1;
		}
		else if (operand->mux == _mux_constant)
		{
			if (have_constant && (constant != operand->index || relative != operand->relative))
				assembler_error(as, "two different constant registers in one instruction");
			constant = operand->index;
			relative = operand->relative;
			have_constant = 1;
		}
	}
	words = as->instructions[as->count++];
	words[0] = 0;
	words[1] = swizzle_bits(operands[0]) | ((DWORD)operands[0]->negate << 8) | ((DWORD)input << 9) |
		((DWORD)constant << 13) | ((DWORD)mac << 21) | ((DWORD)ilu << 25);
	words[2] = ((DWORD)operands[0]->mux << 26) |
		((DWORD)(operands[0]->mux == _mux_temporary ? operands[0]->index : 0) << 28) |
		((DWORD)operands[1]->negate << 25) | (swizzle_bits(operands[1]) << 17) |
		((DWORD)(operands[1]->mux == _mux_temporary ? operands[1]->index : 0) << 13) |
		((DWORD)operands[1]->mux << 11) |
		((DWORD)operands[2]->negate << 10) | (swizzle_bits(operands[2]) << 2) |
		((DWORD)(operands[2]->mux == _mux_temporary ? operands[2]->index : 0) >> 2);
	words[3] = ((DWORD)((operands[2]->mux == _mux_temporary ? operands[2]->index : 0) & 3) << 30) |
		((DWORD)operands[2]->mux << 28) | ((DWORD)relative << 1);
	switch (destination->kind)
	{
	case _dest_temporary:
		if (mac != _mac_nop)
			words[3] |= ((DWORD)destination->mask << 24) | ((DWORD)destination->index << 20);
		else
			words[3] |= ((DWORD)destination->mask << 16) | ((DWORD)destination->index << 20);
		break;
	case _dest_output:
	case _dest_constant:
		words[3] |= ((DWORD)destination->mask << 12) | ((DWORD)(destination->kind == _dest_output) << 11) |
			((DWORD)destination->index << 3) | ((DWORD)(mac == _mac_nop) << 2);
		break;
	case _dest_address:
		/* arl writes a0 itself */
		break;
	}
}

struct opcode
{
	const char *name;
	int mac, ilu;
	/* operands: which of A, B and C take the instruction's sources, in order */
	const char *operands;
};

static const struct opcode opcodes[] =
{
	{ "mov", _mac_mov, _ilu_nop, "A" },
	{ "mul", _mac_mul, _ilu_nop, "AB" },
	{ "add", _mac_add, _ilu_nop, "AC" },
	{ "sub", _mac_add, _ilu_nop, "AC" },
	{ "mad", _mac_mad, _ilu_nop, "ABC" },
	{ "dp3", _mac_dp3, _ilu_nop, "AB" },
	{ "dph", _mac_dph, _ilu_nop, "AB" },
	{ "dp4", _mac_dp4, _ilu_nop, "AB" },
	{ "dst", _mac_dst, _ilu_nop, "AB" },
	{ "min", _mac_min, _ilu_nop, "AB" },
	{ "max", _mac_max, _ilu_nop, "AB" },
	{ "slt", _mac_slt, _ilu_nop, "AB" },
	{ "sge", _mac_sge, _ilu_nop, "AB" },
	{ "rcp", _mac_nop, _ilu_rcp, "C" },
	{ "rcc", _mac_nop, _ilu_rcc, "C" },
	{ "rsq", _mac_nop, _ilu_rsq, "C" },
	{ "expp", _mac_nop, _ilu_exp, "C" },
	{ "exp", _mac_nop, _ilu_exp, "C" },
	{ "logp", _mac_nop, _ilu_log, "C" },
	{ "log", _mac_nop, _ilu_log, "C" },
	{ "lit", _mac_nop, _ilu_lit, "C" },
};

/* splits "a, b, c" in place */
static int split_operands(char *text, char **operands, int maximum)
{
	int count = 0;
	char *p = text;

	while (*p && count < maximum)
	{
		char *start = (char *)skip_spaces(p);
		int depth = 0;

		p = start;
		while (*p && (*p != ',' || depth))
		{
			if (*p == '[')
				depth++;
			else if (*p == ']')
				depth--;
			p++;
		}
		operands[count++] = start;
		if (*p == ',')
			*p++ = 0;
		/* trim the end */
		{
			char *end = start + strlen(start);

			while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
				*--end = 0;
		}
	}
	return count;
}

static void assemble_matrix(struct assembler *as, int rows, int columns, char **operands, int count)
{
	struct destination destination, row_destination;
	struct source vector, matrix;
	int row;

	if (count != 3 || !parse_destination(as, operands[0], &destination) || !parse_source(as, operands[1], &vector) ||
		!parse_source(as, operands[2], &matrix))
	{
		if (count != 3)
			assembler_error(as, "the matrix macro takes three operands");
		return;
	}
	if (matrix.mux != _mux_constant)
	{
		assembler_error(as, "the matrix macro's matrix must be constants");
		return;
	}
	for (row = 0; row < rows; row++)
	{
		struct source row_source = matrix;

		row_destination = destination;
		row_destination.mask = 8 >> row;
		row_source.index = matrix.index + row;
		emit(as, columns == 4 ? _mac_dp4 : _mac_dp3, _ilu_nop, &vector, &row_source, NULL, &row_destination);
	}
}

static void assemble_line(struct assembler *as, char *line)
{
	char *p = (char *)skip_spaces(line), *operands[6];
	char name[16];
	int length = 0, count, index;
	const struct opcode *opcode = NULL;

	if (*p == '+')
		p = (char *)skip_spaces(p + 1);
	while (*p && !isspace((unsigned char)*p) && length < (int)sizeof(name) - 1)
		name[length++] = (char)tolower((unsigned char)*p++);
	name[length] = 0;
	if (!length)
		return;
	count = split_operands(p, operands, 6);

	if (!strcmp(name, "nop"))
		return;
	if (!strcmp(name, "def"))
	{
		/* def c[n], x, y, z, w: kept as a declaration constant token */
		int constant, relative;
		const char *q = operands[0];

		if (count != 5 || tolower((unsigned char)*skip_spaces(q)) != 'c')
		{
			assembler_error(as, "def takes a constant register and four values");
			return;
		}
		q = skip_spaces(q);
		if (!parse_constant(as, &q, &constant, &relative))
			return;
		if (as->constant_words + 5 < sizeof(as->constants) / sizeof(as->constants[0]))
		{
			DWORD *words = &as->constants[as->constant_words];

			words[0] = (4UL << 29) | (1UL << 25) | (DWORD)constant;
			for (index = 0; index < 4; index++)
			{
				float value = strtof(operands[1 + index], NULL);

				memcpy(&words[1 + index], &value, sizeof(value));
			}
			as->constant_words += 5;
		}
		return;
	}
	if (!strcmp(name, "m4x4")) { assemble_matrix(as, 4, 4, operands, count); return; }
	if (!strcmp(name, "m4x3")) { assemble_matrix(as, 3, 4, operands, count); return; }
	if (!strcmp(name, "m3x4")) { assemble_matrix(as, 4, 3, operands, count); return; }
	if (!strcmp(name, "m3x3")) { assemble_matrix(as, 3, 3, operands, count); return; }
	if (!strcmp(name, "m3x2")) { assemble_matrix(as, 2, 3, operands, count); return; }

	for (index = 0; index < (int)(sizeof(opcodes) / sizeof(opcodes[0])); index++)
	{
		if (!strcmp(name, opcodes[index].name))
		{
			opcode = &opcodes[index];
			break;
		}
	}
	if (!opcode)
	{
		assembler_error(as, "unknown instruction '%s'", name);
		return;
	}
	{
		struct destination destination;
		struct source sources[3], *a = NULL, *b = NULL, *c = NULL;
		int operand_count = (int)strlen(opcode->operands);
		int mac = opcode->mac;

		if (count != operand_count + 1)
		{
			assembler_error(as, "'%s' takes %d operands", name, operand_count + 1);
			return;
		}
		if (!parse_destination(as, operands[0], &destination))
			return;
		for (index = 0; index < operand_count; index++)
		{
			if (!parse_source(as, operands[1 + index], &sources[index]))
				return;
			switch (opcode->operands[index])
			{
			case 'A': a = &sources[index]; break;
			case 'B': b = &sources[index]; break;
			default: c = &sources[index]; break;
			}
		}
		if (!strcmp(name, "sub"))
			c->negate ^= 1;
		if (destination.kind == _dest_address)
		{
			if (mac != _mac_mov)
			{
				assembler_error(as, "only mov writes a0.x");
				return;
			}
			mac = _mac_arl;
		}
		emit(as, mac, opcode->ilu, a, b, c, &destination);
	}
}

/* the conversion to the screen the XDK's assembler appends to vs and xvs
programs: oPos.xyz = r12.xyz * c[-38] / r12.w + c[-37] (r12.w kept) */
static void append_screen_space(struct assembler *as)
{
	struct source position = { _mux_temporary, POSITION_TEMPORARY, 0, 0, { 0, 1, 2, 3 } };
	struct source position_w = { _mux_temporary, POSITION_TEMPORARY, 0, 0, { 3, 3, 3, 3 } };
	struct source scale = { _mux_constant, CONSTANT_BIAS - 38, 0, 0, { 0, 1, 2, 3 } };
	struct source offset = { _mux_constant, CONSTANT_BIAS - 37, 0, 0, { 0, 1, 2, 3 } };
	struct source reciprocal = { _mux_temporary, 1, 0, 0, { 0, 0, 0, 0 } };
	struct destination output = { _dest_output, _out_pos, 0xe };
	DWORD *words;

	/* mul oPos.xyz, r12, c[-38] + rcc r1.x, r12.w (the ILU writes r1 when the
	MAC runs too) */
	emit(as, _mac_mul, _ilu_nop, &position, &scale, &position_w, &output);
	if (as->count <= MAX_INSTRUCTIONS)
	{
		words = as->instructions[as->count - 1];
		words[1] |= (DWORD)_ilu_rcc << 25;
		words[3] |= 8UL << 16; /* ILU mask x */
	}
	/* mad oPos.xyz, r12, r1.x, c[-37] */
	emit(as, _mac_mad, _ilu_nop, &position, &reciprocal, &offset, &output);
}

static XGBuffer *text_buffer(const char *text)
{
	return buffer_create(text, strlen(text));
}

/* 0x0022835a */
HRESULT WINAPI XGAssembleShader(LPCSTR source_file_name, LPCVOID source, UINT source_length, DWORD flags,
	XGBuffer **constants, XGBuffer **compiled_shader, XGBuffer **error_log, XGBuffer **listing,
	void *resolver, void *resolver_user_data, DWORD *shader_type)
{
	struct assembler *as = calloc(1, sizeof(*as));
	const char *text = source;
	unsigned long position = 0;
	BOOL have_version = FALSE;
	char line[512];
	HRESULT result = S_OK;

	(void)flags;
	(void)resolver;
	(void)resolver_user_data;
	if (constants) *constants = NULL;
	if (compiled_shader) *compiled_shader = NULL;
	if (error_log) *error_log = NULL;
	if (listing) *listing = NULL;
	if (!as)
		return E_OUTOFMEMORY;
	if (!text)
	{
		free(as);
		return E_FAIL;
	}
	as->type = 0x2078;
	as->screen_space = TRUE;
	while (position < source_length && text[position])
	{
		unsigned long length = 0;
		char *comment;

		as->line++;
		while (position < source_length && text[position] && text[position] != '\n')
		{
			if (length < sizeof(line) - 1)
				line[length++] = text[position];
			position++;
		}
		if (position < source_length && text[position] == '\n')
			position++;
		line[length] = 0;
		if ((comment = strchr(line, ';')) != NULL)
			*comment = 0;
		if ((comment = strstr(line, "//")) != NULL)
			*comment = 0;
		if (!*skip_spaces(line))
			continue;
		if (!have_version)
		{
			const char *p = skip_spaces(line);

			have_version = TRUE;
			if (!strncasecmp(p, "xvss.", 5))
			{
				as->type = 0x7378;
				as->screen_space = FALSE;
				continue;
			}
			if (!strncasecmp(p, "xvsw.", 5))
			{
				as->type = 0x7778;
				as->screen_space = FALSE;
				continue;
			}
			if (!strncasecmp(p, "xvs.", 4) || !strncasecmp(p, "vs.", 3))
				continue;
			if (!strncasecmp(p, "ps.", 3) || !strncasecmp(p, "xps.", 4))
			{
				assembler_error(as, "pixel shaders are not supported");
				break;
			}
			assembler_error(as, "the shader has no version (vs.1.1, xvs.1.1, ...)");
			break;
		}
		assemble_line(as, line);
	}
	if (!have_version && !as->failed)
		assembler_error(as, "empty shader");
	if (!as->failed && as->screen_space && as->writes_position)
		append_screen_space(as);
	if (!as->failed && !as->count)
		assembler_error(as, "the shader has no instructions");
	if (as->failed)
	{
		platform_log("XGAssembleShader %s: %s", source_file_name ? source_file_name : "", as->errors);
		result = E_FAIL;
	}
	else
	{
		DWORD *program;
		unsigned long size = 4 + as->count * 16;

		as->instructions[as->count - 1][3] |= 1; /* the end */
		program = malloc(size);
		if (!program)
		{
			free(as);
			return E_OUTOFMEMORY;
		}
		program[0] = ((DWORD)as->count << 16) | as->type;
		memcpy(program + 1, as->instructions, as->count * 16);
		if (compiled_shader)
			*compiled_shader = buffer_create(program, size);
		free(program);
	}
	/* the game releases every buffer it asked for, unchecked */
	if (constants)
	{
		as->constants[as->constant_words++] = D3DVSD_END();
		*constants = buffer_create(as->constants, as->constant_words * 4);
	}
	if (error_log)
		*error_log = text_buffer(as->errors);
	if (listing)
		*listing = text_buffer("");
	if (shader_type)
		*shader_type = as->type;
	free(as);
	return result;
}
