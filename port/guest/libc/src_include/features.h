/* Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) port/android/guest/libc/src_include/features.h (CC0). Renamed HALO_* to OPENFRENZY_*. */
/*
musl's internal features.h, with weak_alias spelled as assembler
directives: the guest is compiled for a Darwin target, which rejects alias
attributes, and its assembly is converted to ELF afterwards
(tools/android_asm_convert.py), where the alias is ordinary.
*/

#ifndef GUEST_FEATURES_H
#define GUEST_FEATURES_H

#include_next "features.h"

#undef weak_alias
#define weak_alias(old, new) \
	extern __typeof(old) new; \
	static void *const __guest_alias_##new __attribute__((used)) = (void *)&old; \
	__asm__(".globl _" #new "\n.weak_definition _" #new "\n.set _" #new ", _" #old "\n")

#endif
