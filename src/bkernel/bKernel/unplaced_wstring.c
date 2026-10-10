/*
bKernel wide (UCS-2, 16-bit) strings: copy, length and a small sprintf for the game's
on-screen text.

bkWideSprintf understands %[0][width][.precision] with c C d i u f e E g G s S and l
(ld li lu lc lC le lf lg ls lS); %s takes a narrow string, %S and %ls a wide one. Floating
point is written digit by digit with truncation (no rounding), as in the original.

The original source file is not known (gap functions in C:\Babel\Xbox\Src\bKernel).
*/

#include "bkernel_core.h"

#include <math.h>

/* digits of a non-negative int (0 has none) */
static int bkDigitCount(int value)
{
	int count = 0;

	for (; value > 0; value /= 10)
		count++;
	return count;
}

/* writes count copies of c */
static ushort *bkWideFill(ushort *out, ushort c, int count)
{
	for (; count > 0; count--)
		*out++ = c;
	return out;
}

/* writes digits of a mantissa backward from the last position: total characters, with
'.' after precision digits (precision < 0: none); x is scaled so that its last digit is
the units. Returns the position after the last character. */
static ushort *bkWideMantissa(ushort *out, double x, int total, int point)
{
	ushort *p = out + total - 1;
	int index;

	for (index = 0; index < total; index++, p--)
	{
		if (index == point)
		{
			*p = '.';
		}
		else
		{
			*p = (ushort)(int)(floor(fmod(x, 10.0)) + 48.0);
			x *= 0.1;
		}
	}
	return p + total + 1;
}

/* writes an exponent: e or E, its sign and at least three digits */
static ushort *bkWideExponent(ushort *out, ushort exponentChar, int exponent)
{
	int digits;
	int width;
	ushort *p;

	*out++ = exponentChar;
	if (exponent < 0)
	{
		*out++ = '-';
		exponent = -exponent;
	}
	else
	{
		*out++ = '+';
	}
	digits = bkDigitCount(exponent);
	width = digits < 3 ? 3 : digits;
	out = bkWideFill(out, '0', width - digits);
	p = out + digits - 1;
	for (; exponent > 0; exponent /= 10)
		*p-- = (ushort)('0' + exponent % 10);
	return p + digits + 1;
}

/* writes an integer: a minus sign, then padding to width, zeros to minDigits, digits.
Signed values count and write their digits as int (so INT_MIN has none, as in the
original); unsigned ones as uint. */
static ushort *bkWideInteger(ushort *out, uint value, int isSigned, int width, int minDigits,
	byte padChar)
{
	int digits = 0;
	int zeros;
	ushort *p;

	if (isSigned)
	{
		int number = (int)value;
		int rest;

		if (number < 0)
		{
			*out++ = '-';
			number = -number;
		}
		for (rest = number; rest > 0; rest /= 10)
			digits++;
		if (minDigits <= digits)
			minDigits = digits;
		if (width < minDigits)
			width = minDigits;
		zeros = minDigits - digits;
		out = bkWideFill(out, padChar, width - zeros - digits);
		out = bkWideFill(out, '0', zeros);
		p = out + digits - 1;
		for (; number > 0; number /= 10)
			*p-- = (ushort)('0' + number % 10);
		return p + digits + 1;
	}
	{
		uint rest;

		for (rest = value; rest != 0; rest /= 10)
			digits++;
		if (minDigits <= digits)
			minDigits = digits;
		if (width < minDigits)
			width = minDigits;
		zeros = minDigits - digits;
		out = bkWideFill(out, padChar, width - zeros - digits);
		out = bkWideFill(out, '0', zeros);
		p = out + digits - 1;
		for (; value != 0; value /= 10)
			*p-- = (ushort)('0' + value % 10);
		return p + digits + 1;
	}
}

/* 0x0004a330
bkStringToWide: copies a narrow string into a wide one. */
void bkStringToWide(ushort *wide, const char *string)
{
	for (; *string != '\0'; string++)
		*wide++ = (byte)*string;
	*wide = 0;
}

/* 0x0004a360 (static in practice; called by bkWideSprintf)
bkWideFormatArgument: formats one argument. type: 1 f, 2 e, 3 g, 4 d, 5 u, 6 ld, 7 lu,
8 narrow s, 9 wide s, 10 c, 11 C. Returns the position after it, or NULL for an unknown
type. */
ushort *bkWideFormatArgument(ushort *out, int type, int width, int precision, int hasPrecision,
	byte padChar, ushort exponentChar, char conversion, __builtin_va_list *arguments)
{
	double x;
	double y;
	int digits;
	int exponent;
	int total;
	int value;
	const char *string;
	const ushort *wide;

	(void)conversion;
	switch (type)
	{
	case 1: /* f */
		if (hasPrecision == 0)
			precision = 6;
		x = __builtin_va_arg(*arguments, double);
		if (x < 0.0)
		{
			x = -x;
			*out++ = '-';
		}
		digits = 0;
		if (x >= 1.0)
		{
			for (y = x; y >= 1.0; y *= 0.1)
				digits++;
		}
		if (digits == 0)
			digits = 1;
		total = digits + 1 + precision;
		return bkWideMantissa(out, pow(10.0, precision) * x, total, precision);

	case 2: /* e */
		if (hasPrecision == 0)
			precision = 6;
		x = __builtin_va_arg(*arguments, double);
		if (x < 0.0)
		{
			x = -x;
			*out++ = '-';
		}
		exponent = 0;
		if (x >= 1.0)
		{
			for (; x >= 10.0; x *= 0.1)
				exponent++;
		}
		else if (x != 0.0)
		{
			while (x < 1.0)
			{
				x *= 10.0;
				exponent--;
				if (x == 0.0)
					break;
			}
		}
		out = bkWideMantissa(out, pow(10.0, precision) * x, precision + 2, precision);
		return bkWideExponent(out, exponentChar, exponent);

	case 3: /* g */
		if (hasPrecision == 0)
			precision = 6;
		x = __builtin_va_arg(*arguments, double);
		if (x < 0.0)
		{
			x = -x;
			*out++ = '-';
		}
		digits = 0;
		if (x >= 1.0)
		{
			for (y = x; y >= 1.0; y *= 0.1)
				digits++;
		}
		if (digits == 0)
			digits = 1;
		exponent = 0;
		if (x >= 1.0)
		{
			if (pow(10.0, precision) < x)
			{
				for (; x >= 10.0; x *= 0.1)
					exponent++;
			}
		}
		else if (x != 0.0)
		{
			while (x < 1.0)
			{
				x *= 10.0;
				exponent--;
				if (x == 0.0)
					break;
			}
		}
		total = digits + (precision - exponent) + 1;
		x *= pow(10.0, precision);
		value = precision;
		/* drop trailing zeros of the fraction */
		while (total > 1 && value != 0 && fmod(x, 10.0) == 0.0)
		{
			x *= 0.1;
			total--;
			value--;
		}
		if (value == 0)
			total--; /* no fraction: no point */
		out = bkWideMantissa(out, x, total, value != 0 ? value : -1);
		if (exponent == 0)
			return out;
		return bkWideExponent(out, exponentChar, exponent);

	case 4: /* d */
	case 6: /* ld */
		return bkWideInteger(out, __builtin_va_arg(*arguments, uint), 1, width,
			hasPrecision ? precision : 1, padChar);

	case 5: /* u */
	case 7: /* lu */
		return bkWideInteger(out, __builtin_va_arg(*arguments, uint), 0, width,
			hasPrecision ? precision : 1, padChar);

	case 8: /* narrow string */
		string = __builtin_va_arg(*arguments, const char *);
		if (hasPrecision == 0)
		{
			for (; *string != '\0'; string++)
				*out++ = (byte)*string;
			return out;
		}
		for (; *string != '\0' && precision != 0; string++, precision--)
			*out++ = (byte)*string;
		return out;

	case 9: /* wide string */
		wide = __builtin_va_arg(*arguments, const ushort *);
		if (hasPrecision == 0)
		{
			for (; *wide != 0; wide++)
				*out++ = *wide;
			return out;
		}
		for (; *wide != 0 && precision != 0; wide++, precision--)
			*out++ = *wide;
		return out;

	case 10: /* c */
	case 11: /* C */
		*out++ = (ushort)__builtin_va_arg(*arguments, int);
		return out;

	default:
		*out = 0;
		return NULL;
	}
}

/* 0x0004ae80
bkWideSprintf: sprintf into a wide string from a wide format. Returns the length, or 0 on
an unknown conversion. */
int bkWideSprintf(ushort *out, const ushort *format, ...)
{
	__builtin_va_list arguments;
	ushort *start = out;
	const ushort *p;
	ushort c;
	int width;
	int precision;
	int hasPrecision;
	byte padChar;
	ushort exponentChar;
	int type;

	__builtin_va_start(arguments, format);
	for (;;)
	{
		c = *format;
		if (c == 0)
		{
			*out = 0;
			__builtin_va_end(arguments);
			return (int)(out - start);
		}
		if ((char)c != '%')
		{
			*out++ = c;
			format++;
			continue;
		}
		p = format + 1;
		c = *p;
		width = 0;
		padChar = c == '0' ? '0' : ' ';
		while (c >= '0' && c <= '9')
		{
			p++;
			width = width * 10 + (c - '0');
			c = *p;
		}
		precision = 0;
		hasPrecision = 0;
		if (*p == '.')
		{
			p++;
			c = *p;
			while (c >= '0' && c <= '9')
			{
				p++;
				precision = precision * 10 + (c - '0');
				c = *p;
			}
			hasPrecision = 1;
		}
		format = p + 1;
		exponentChar = 0;
		type = 0;
		switch ((char)*p)
		{
		case 'C':
			type = 11;
			break;
		case 'E':
		case 'e':
			exponentChar = format[-1];
			type = 2;
			break;
		case 'G':
		case 'g':
			exponentChar = (ushort)(format[-1] - 2);
			type = 3;
			break;
		case 'S':
			type = 9;
			break;
		case 'c':
			type = 10;
			break;
		case 'd':
		case 'i':
			type = 4;
			break;
		case 'f':
			type = 1;
			break;
		case 'l':
			c = *format;
			format = p + 2;
			switch (c)
			{
			case 'C':
			case 'c':
				type = 11;
				break;
			case 'E':
			case 'e':
				exponentChar = format[-1];
				type = 2;
				break;
			case 'G':
			case 'g':
				exponentChar = (ushort)(format[-1] - 2);
				type = 3;
				break;
			case 'S':
			case 's':
				type = 9;
				break;
			case 'd':
			case 'i':
				type = 6;
				break;
			case 'f':
				type = 1;
				break;
			case 'u':
				type = 7;
				break;
			}
			break;
		case 's':
			type = 8;
			break;
		case 'u':
			type = 5;
			break;
		}
		out = bkWideFormatArgument(out, type, width, precision, hasPrecision, padChar, exponentChar,
			(char)format[-1], &arguments);
		if (out == NULL)
		{
			__builtin_va_end(arguments);
			return 0;
		}
	}
}

/* 0x0004b110 */
int bkWideStrlen(const ushort *string)
{
	const ushort *end = string;

	while (*end++ != 0)
		;
	return (int)(end - string) - 1;
}

/* 0x0004b130 */
void bkWideStrcpy(ushort *destination, const ushort *source)
{
	ushort c;

	do
	{
		c = *source++;
		*destination++ = c;
	} while (c != 0);
}
