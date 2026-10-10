/*
TEST_TOOLCHAIN_STRINGS.C

String literals must reach the guest image unchanged. port/macos/tools/asm_convert.py
(Mach-O to ELF assembly) renames symbols by dropping Darwin's leading '_', and it also
applies that to quoted .asciz operands: "_stricmp" became "stricmp" in the image
(compare <test>.darwin.s with <test>.elf.s). Game strings that start with '_' (file
names, format strings, debug text) would change. Listed in known_failures.txt until the
architect fixes the converter.
*/

#include <stdio.h>
#include <string.h>

#include "of_test.h"

static const char underscore_global[] = "_T_E_S_T.___";

/* read through a volatile pointer, so the compiler cannot fold the check at build time */
static int first_byte(const char *text)
{
	const char *volatile pointer = text;

	return (unsigned char)pointer[0];
}

static unsigned int runtime_length(const char *text)
{
	const char *volatile pointer = text;
	unsigned int length = 0;

	while (pointer[length])
		length++;
	return length;
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	OF_CHECK(first_byte("_leading") == '_' && runtime_length("_leading") == 8,
		"a literal keeps its leading '_' (\"%s\")", "_leading");
	OF_CHECK(first_byte(underscore_global) == '_' && runtime_length(underscore_global) == 12,
		"an initialised array keeps its leading '_' (\"%s\")", underscore_global);
	OF_CHECK(runtime_length("__x") == 3, "\"__x\" keeps both '_' (\"%s\")", "__x");
	OF_CHECK(runtime_length("a_b") == 3, "an inner '_' is unchanged (\"%s\")", "a_b");
	return of_test_result("test_toolchain_strings");
}
