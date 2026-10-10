/*
TEST_MSVC_CRT.C

msvc_crt.c and msvc_wide.c: the Microsoft C runtime behaviour that the game's
decompiled code relies on, with the formats and calls the game really makes.

- Text is 16-bit (wchar_t is 2 bytes with -fshort-wchar, as on the Xbox). The
  game builds its menu text with a wide sprintf (FUN_0004ae80) and formats like
  L"%S: %S" and L"%S %d" (export/funcs/0001cdf0.c, 0002b9e0.c), where MSVC's
  wide %S takes a NARROW string and %s a wide one: the opposite of ISO C.
- The WMA streamer logs "%I64d ticks" (0x00056d20): MSVC's 64-bit modifier.
- bKernel compares package and file names with _stricmp/_strnicmp and
  builds numbers with _itoa (docs/LIBRARY_CALLS.md).
*/

#include "platform.h"
#include "of_test.h"

#include <string.h>
#include <wchar.h>

int halo_linux_sprintf(char *buffer, const char *format, ...);
int halo_linux_snprintf(char *buffer, size_t count, const char *format, ...);

static int wide_equals(const wchar_t *wide, const char *narrow)
{
	while (*narrow)
	{
		if (*wide++ != (wchar_t)(unsigned char)*narrow++)
			return 0;
	}
	return *wide == 0;
}

int main(void)
{
	wchar_t wide[64];
	char text[128];
	char drive[8], directory[64], name[32], extension[16];
	long long ticks = 0x123456789LL;

	setvbuf(stdout, NULL, _IONBF, 0);
	OF_CHECK(sizeof(wchar_t) == 2, "wchar_t is 16 bits, as on the Xbox (%u)", (unsigned)sizeof(wchar_t));

	/* wide printf with MSVC's %S (narrow) and %s (wide) */
	swprintf(wide, L"%S: %S", "Player 1", "Ready");
	OF_CHECK(wide_equals(wide, "Player 1: Ready"), "swprintf L\"%%S: %%S\" with narrow strings");
	swprintf(wide, L"%S %d", "Round", 3);
	OF_CHECK(wide_equals(wide, "Round 3"), "swprintf L\"%%S %%d\"");
	swprintf(wide, L"%s/%S", L"wide", "narrow");
	OF_CHECK(wide_equals(wide, "wide/narrow"), "swprintf: %%s is wide, %%S narrow");
	OF_CHECK(wcslen(L"Fuzion") == 6 && !wcscmp(L"abc", L"abc") && wcscmp(L"abc", L"abd") < 0,
		"wcslen/wcscmp on 16-bit strings");
	OF_CHECK(_wcsicmp(L"BugBash", L"BUGBASH") == 0, "_wcsicmp ignores case");

	/* narrow printf with MSVC length modifiers */
	halo_linux_sprintf(text, "%.2f fps, %I64d ticks", 59.94, ticks);
	OF_CHECK(!strcmp(text, "59.94 fps, 4886718345 ticks"), "%%I64d prints a 64-bit value (%s)", text);
	halo_linux_sprintf(text, "%I32x", 0xbeefu);
	OF_CHECK(!strcmp(text, "beef"), "%%I32x is %%x (%s)", text);
	OF_CHECK(halo_linux_snprintf(text, 4, "%s", "Packages") == 8 && !strcmp(text, "Pac"),
		"_snprintf truncates and terminates (%s)", text);

	/* names: packages are found case-insensitively */
	OF_CHECK(_stricmp("Packages\\BootSeq.xbp", "PACKAGES\\bootseq.XBP") == 0, "_stricmp ignores case");
	OF_CHECK(_stricmp("a", "B") < 0 && _stricmp("b", "A") > 0, "_stricmp orders letters without case");
	OF_CHECK(_strnicmp("carryon_v1", "CARRYON_V2", 9) == 0 && _strnicmp("carryon_v1", "CARRYON_V2", 10) != 0,
		"_strnicmp stops at the count");
	OF_CHECK(!strcmp(_itoa(-42, text, 10), "-42") && !strcmp(_itoa(255, text, 16), "ff"), "_itoa in base 10 and 16");
	strcpy(text, "Music.XBP");
	OF_CHECK(!strcmp(_strlwr(text), "music.xbp"), "_strlwr");

	_splitpath("d:\\Packages\\English\\CharVoice.xbp", drive, directory, name, extension);
	OF_CHECK(!strcmp(drive, "d:") && !strcmp(directory, "\\Packages\\English\\") && !strcmp(name, "CharVoice") &&
		!strcmp(extension, ".xbp"), "_splitpath of a disc path (%s|%s|%s|%s)", drive, directory, name, extension);

	return of_test_result("test_msvc_crt");
}
