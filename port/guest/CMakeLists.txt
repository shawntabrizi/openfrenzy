# Guest images (arm64_32, rebased into the host's 4 GB arena). See docs/ARCHITECTURE.md and docs/BUILD.md.
# Built by the top-level CMakeLists.txt as an ExternalProject with cmake/guest-toolchain.cmake.
#
# Layout and flags follow OpenCE tools/android_build.py (ref/OpenCE-mac-zimm3rmann @ e55684a, CC0):
#   libguestc.a           musl subset (the guest C library)
#   guest_runtime         port/guest/runtime + generated GL/posix import wrappers + host import stubs
#   libofplatform.a       the XDK replacement layer: src/{runtime,vfs,xkernel,d3d8,dsound,input}
#   openfrenzy_smoke.elf  platform smoke test: SDL3 window + GLES clear + a file read from the disc image
#   openfrenzy_guest.elf  the game: every other src/<module>/ + generated weak stubs + src/runtime boot
cmake_minimum_required(VERSION 3.24)
project(openfrenzy_guest C ASM)

foreach(_var OPENFRENZY_MUSL_DIR OPENFRENZY_MUSL_VERSION OPENFRENZY_GL_INCLUDE OPENFRENZY_SDL_INCLUDE)
  if(NOT ${_var})
    message(FATAL_ERROR "port/guest: ${_var} is not set")
  endif()
endforeach()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(R "${OPENFRENZY_ROOT}")
set(M "${OPENFRENZY_MUSL_DIR}")
set(ARCH "${R}/port/guest/libc/arch/arm64_32")
set(RUNTIME_DIR "${R}/port/guest/runtime")
set(SRC "${R}/src")
set(GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(LIBC_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/libc_include")
set(LIBC_INTERNAL "${CMAKE_CURRENT_BINARY_DIR}/libc_internal")
file(MAKE_DIRECTORY "${GEN}" "${LIBC_INCLUDE}/bits" "${LIBC_INTERNAL}")

# Platform modules (XDK replacement). Every other directory under src/ is a game/engine module.
set(OPENFRENZY_PLATFORM_MODULES runtime vfs xkernel d3d8 dsound input)

# ---------- generated musl headers (as android_build.py)

add_custom_command(OUTPUT "${LIBC_INCLUDE}/bits/alltypes.h"
  COMMAND sed -f "${M}/tools/mkalltypes.sed" "${ARCH}/bits/alltypes.h.in" "${M}/include/alltypes.h.in"
          > "${LIBC_INCLUDE}/bits/alltypes.h"
  DEPENDS "${ARCH}/bits/alltypes.h.in")
add_custom_command(OUTPUT "${LIBC_INCLUDE}/bits/syscall.h"
  COMMAND cp "${ARCH}/bits/syscall.h.in" "${LIBC_INCLUDE}/bits/syscall.h"
  COMMAND sed -n -e s/__NR_/SYS_/p < "${ARCH}/bits/syscall.h.in" >> "${LIBC_INCLUDE}/bits/syscall.h"
  DEPENDS "${ARCH}/bits/syscall.h.in")
file(WRITE "${LIBC_INTERNAL}/version.h" "#define VERSION \"${OPENFRENZY_MUSL_VERSION}\"\n")
# MSVC linkage semantics for the XDK headers' inline functions (OpenCE linux_msvc_semantics.py)
add_custom_command(OUTPUT "${GEN}/platform_msvc_semantics.h"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/scripts/linux_msvc_semantics.py"
          --output "${GEN}/platform_msvc_semantics.h" --inlines "${SRC}/runtime/include/xdk"
  DEPENDS "${R}/scripts/linux_msvc_semantics.py")
add_custom_target(guest_headers DEPENDS "${LIBC_INCLUDE}/bits/alltypes.h" "${LIBC_INCLUDE}/bits/syscall.h"
  "${GEN}/platform_msvc_semantics.h")

# "SHELL:" keeps CMake from merging the repeated -isystem / -include options
set(LIBC_SYSTEM_INCLUDES "SHELL:-isystem ${LIBC_INCLUDE}" "SHELL:-isystem ${ARCH}"
    "SHELL:-isystem ${M}/arch/generic" "SHELL:-isystem ${M}/include")
set(GUEST_CODE_FLAGS -nostdinc -fms-extensions -fcommon -fno-strict-aliasing -fwrapv
    -fno-delete-null-pointer-checks -fno-omit-frame-pointer)
foreach(_n wcslen wcsnlen wcschr wcsrchr wcscmp wcsncmp wcscpy wcsncpy wcscat wcsncat wmemchr wmemcmp
           wmemcpy wmemmove wmemset)
  list(APPEND GUEST_CODE_FLAGS -fno-builtin-${_n})
endforeach()

# ---------- libguestc.a: musl (source list from android_build.py)

set(_musl_dirs conf ctype dirent env errno exit fcntl internal locale malloc malloc/mallocng math mman
    multibyte prng sched select signal stat stdio stdlib string time unistd)
set(_musl_sources)
foreach(_d ${_musl_dirs})
  file(GLOB _f "${M}/src/${_d}/*.c")
  list(APPEND _musl_sources ${_f})
endforeach()
foreach(_f thread/__lock.c thread/__wait.c thread/__timedwait.c thread/__syscall_cp.c thread/vmlock.c
           thread/pthread_self.c thread/pthread_equal.c thread/pthread_once.c
           thread/pthread_setcancelstate.c thread/pthread_testcancel.c thread/default_attr.c
           thread/lock_ptc.c misc/getauxval.c linux/sysinfo.c misc/basename.c misc/dirname.c
           misc/realpath.c misc/uname.c misc/ioctl.c misc/getrlimit.c misc/syscall.c)
  list(APPEND _musl_sources "${M}/src/${_f}")
endforeach()
file(GLOB _thread "${M}/src/thread/pthread_attr_*.c" "${M}/src/thread/pthread_cond*.c"
     "${M}/src/thread/pthread_mutex*.c" "${M}/src/thread/pthread_rwlock*.c"
     "${M}/src/thread/pthread_spin*.c" "${M}/src/thread/sem_*.c")
list(APPEND _musl_sources ${_thread})
list(REMOVE_DUPLICATES _musl_sources)
# replaced by the guest runtime (port/guest/runtime)
list(FILTER _musl_sources EXCLUDE REGEX
     "/src/(env/__stack_chk|env/__init_tls|env/__libc_start_main|env/__reset_tls|string/explicit_bzero)\\.c$")
list(FILTER _musl_sources EXCLUDE REGEX "/src/malloc/oldmalloc/")
# faster replacements (port/guest/libc/string)
list(FILTER _musl_sources EXCLUDE REGEX "/src/string/memcmp\\.c$")
list(APPEND _musl_sources "${R}/port/guest/libc/string/memcmp.c")

add_library(guestc STATIC ${_musl_sources})
add_dependencies(guestc guest_headers)
target_compile_options(guestc PRIVATE -nostdinc -std=c99 -ffreestanding -fno-common -D_XOPEN_SOURCE=700 -w
  -I${ARCH} -I${M}/arch/generic -I${LIBC_INTERNAL} -I${R}/port/guest/libc/src_include
  -I${M}/src/include -I${M}/src/internal -I${LIBC_INCLUDE} -I${M}/include)

# ---------- host imports, GL and posix wrappers

set(_posix_h "${SRC}/vfs/posix.h")
add_custom_command(OUTPUT "${GEN}/guest_posix.c" "${GEN}/posix_imports.list"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/posix_stubs.py" "${_posix_h}"
          "${GEN}/guest_posix.c" "${GEN}/posix_imports.list"
  DEPENDS "${R}/port/macos/tools/posix_stubs.py" "${_posix_h}")
add_custom_command(OUTPUT "${GEN}/guest_gl.c" "${GEN}/gl_imports.list"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/gl_stubs.py" "${SRC}/d3d8/gl.h"
          "${OPENFRENZY_GL_INCLUDE}/GLES3/gl32.h" "${OPENFRENZY_GL_INCLUDE}/GLES2/gl2ext.h"
          "${GEN}/guest_gl.c" "${GEN}/gl_imports.list"
  DEPENDS "${R}/port/macos/tools/gl_stubs.py" "${SRC}/d3d8/gl.h")
add_custom_command(OUTPUT "${GEN}/imports.s"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/host_imports.py" "${GEN}/imports.s"
          "${R}/port/macos/host_imports.list" "${GEN}/posix_imports.list" "${GEN}/gl_imports.list"
  DEPENDS "${R}/port/macos/tools/host_imports.py" "${R}/port/macos/host_imports.list"
          "${GEN}/posix_imports.list" "${GEN}/gl_imports.list")

set(RUNTIME_FLAGS ${GUEST_CODE_FLAGS} -std=gnu11 -D_GNU_SOURCE -w
    -I${RUNTIME_DIR} -I${R}/port/macos/include -I${SRC}/vfs -I${SRC}/runtime
    -I${OPENFRENZY_SDL_INCLUDE} -I${OPENFRENZY_GL_INCLUDE} ${LIBC_SYSTEM_INCLUDES})
set(RUNTIME_INTERNAL_FLAGS -nostdinc -std=c99 -ffreestanding -fno-common -D_XOPEN_SOURCE=700 -D_GNU_SOURCE -w
    -I${RUNTIME_DIR} -I${R}/port/macos/include -I${ARCH} -I${M}/arch/generic -I${LIBC_INTERNAL}
    -I${R}/port/guest/libc/src_include -I${M}/src/include -I${M}/src/internal -I${LIBC_INCLUDE}
    -I${M}/include)

add_library(guest_runtime OBJECT
  "${RUNTIME_DIR}/guest_misc.c" "${RUNTIME_DIR}/guest_sdl.c" "${RUNTIME_DIR}/guest_syscall.c"
  "${RUNTIME_DIR}/guest_start.c" "${RUNTIME_DIR}/guest_thread.c"
  "${GEN}/guest_gl.c" "${GEN}/guest_posix.c" "${GEN}/imports.s")
add_dependencies(guest_runtime guest_headers)
target_compile_options(guest_runtime PRIVATE $<$<COMPILE_LANGUAGE:C>:${RUNTIME_FLAGS}>)
set_source_files_properties("${RUNTIME_DIR}/guest_start.c" "${RUNTIME_DIR}/guest_thread.c"
  PROPERTIES COMPILE_OPTIONS "${RUNTIME_INTERNAL_FLAGS}")

# ---------- the platform layer (XDK replacement), shared include path for all src/ code

set(PLATFORM_INCLUDES)
foreach(_m ${OPENFRENZY_PLATFORM_MODULES})
  list(APPEND PLATFORM_INCLUDES -I${SRC}/${_m})
endforeach()
list(APPEND PLATFORM_INCLUDES -I${SRC}/runtime/compat -I${SRC}/runtime/include/linux
     -I${SRC}/runtime/include -I${SRC}/runtime/include/xdk -I${SRC}/runtime/entry -I${RUNTIME_DIR}
     -I${R}/port/macos/include -I${OPENFRENZY_SDL_INCLUDE} -I${OPENFRENZY_GL_INCLUDE})
set(PLATFORM_FLAGS ${GUEST_CODE_FLAGS} -std=gnu11 -D_GNU_SOURCE -DHALO_LINUX_PLATFORM_LAYER
    -DHALO_ARM64_GUEST=1 -DHALO_GLES=1 -DOPENFRENZY_GUEST=1 -w
    "SHELL:-include ${SRC}/runtime/include/linux/halo_linux_prefix.h"
    "SHELL:-include ${GEN}/platform_msvc_semantics.h"
    ${PLATFORM_INCLUDES} ${LIBC_SYSTEM_INCLUDES})

set(_platform_sources)
foreach(_m ${OPENFRENZY_PLATFORM_MODULES})
  file(GLOB _f CONFIGURE_DEPENDS "${SRC}/${_m}/*.c" "${SRC}/${_m}/compat/*.c" "${SRC}/${_m}/entry/*.c")
  list(APPEND _platform_sources ${_f})
endforeach()
# the game's boot code is linked only into the game image
list(FILTER _platform_sources EXCLUDE REGEX "/src/runtime/(of_boot|of_main)\\.c$")
# the guest side of memory write tracking (calls the host) belongs with the layer
list(APPEND _platform_sources "${RUNTIME_DIR}/guest_memory_watch.c")
# SMAA's shader and lookup textures as C data (src/d3d8/xgpu_post.c; MIT, port/third_party/smaa)
set(_smaa "${R}/port/third_party/smaa")
add_custom_command(OUTPUT "${GEN}/xgpu_smaa.c"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/embed_smaa.py" "${_smaa}" "${GEN}/xgpu_smaa.c"
  DEPENDS "${R}/port/macos/tools/embed_smaa.py" "${_smaa}/SMAA.hlsl" "${_smaa}/area_tex.zlib"
          "${_smaa}/search_tex.zlib")
list(APPEND _platform_sources "${GEN}/xgpu_smaa.c")
add_library(ofplatform STATIC ${_platform_sources})
add_dependencies(ofplatform guest_headers)
target_compile_options(ofplatform PRIVATE ${PLATFORM_FLAGS})

# ---------- images

set(_link_libs ofplatform guestc)
function(of_guest_image name)
  add_executable(${name} ${ARGN} $<TARGET_OBJECTS:guest_runtime>)
  add_dependencies(${name} guest_runtime)
  target_link_libraries(${name} PRIVATE ${_link_libs})
  set_target_properties(${name} PROPERTIES SUFFIX ".elf" LINK_DEPENDS "${OPENFRENZY_GUEST_LD_SCRIPT}")
endfunction()

# link check: every platform object linked whole, so a symbol the layer needs and nobody
# defines fails the build now, not later when game code first calls into that file.
add_executable(openfrenzy_platform_check "${R}/port/macos/tests/smoke/platform_check.c"
  $<TARGET_OBJECTS:guest_runtime>)
add_dependencies(openfrenzy_platform_check guest_runtime)
target_compile_options(openfrenzy_platform_check PRIVATE ${PLATFORM_FLAGS})
target_link_libraries(openfrenzy_platform_check PRIVATE "-whole-archive" ofplatform "-no-whole-archive" guestc)
set_target_properties(openfrenzy_platform_check PROPERTIES SUFFIX ".elf")

# smoke test: window + GLES + disc image read (port/macos/tests/smoke)
of_guest_image(openfrenzy_smoke "${R}/port/macos/tests/smoke/smoke.c")
target_compile_options(openfrenzy_smoke PRIVATE ${PLATFORM_FLAGS})

# ---------- the game image: game/engine modules + generated declarations and weak stubs

file(GLOB _src_dirs LIST_DIRECTORIES true RELATIVE "${SRC}" "${SRC}/*")
set(_game_modules)
foreach(_d ${_src_dirs})
  if(IS_DIRECTORY "${SRC}/${_d}" AND NOT _d IN_LIST OPENFRENZY_PLATFORM_MODULES AND NOT _d STREQUAL "include")
    list(APPEND _game_modules ${_d})
  endif()
endforeach()
set(_game_sources)
set(GAME_INCLUDES -I${SRC}/include -I${GEN})
foreach(_m ${_game_modules})
  file(GLOB_RECURSE _f CONFIGURE_DEPENDS "${SRC}/${_m}/*.c")
  list(APPEND _game_sources ${_f})
  list(APPEND GAME_INCLUDES -I${SRC}/${_m})
endforeach()
file(GLOB_RECURSE _decl_files CONFIGURE_DEPENDS "${SRC}/*.decls")

set(_table "${SRC}/include/of_functions.tsv")
if(NOT EXISTS "${_table}")
  message(WARNING "${_table} is missing: no game image (run scripts/gen_function_table.py)")
  return()
endif()
add_custom_command(OUTPUT "${GEN}/of_decls.h" "${GEN}/of_stubs.c" "${GEN}/of_crt_init.c"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/gen_game_decls.py"
          --table "${_table}" --src "${SRC}" --out "${GEN}"
  DEPENDS "${R}/port/macos/tools/gen_game_decls.py" "${_table}" ${_decl_files}
          "${SRC}/include/of_types.h"
  COMMENT "Game declarations and weak stubs")
add_custom_target(game_decls DEPENDS "${GEN}/of_decls.h")

# Game/engine code: the platform flags (so a file may include "xtl.h" for XDK types) plus
# of_game.h. Warnings stay on, except the ones that Ghidra-shaped code triggers everywhere.
# int<->pointer conversions and object and function pointer type mismatches are errors (the
# audits in docs/INTEGRATION_NOTES.md made all of them explicit). A -Wno-incompatible-pointer-types
# would also turn off its subgroup incompatible-function-pointer-types: do not add one.
# Operator grouping (-Wparentheses) is an error too: every grouping is explicit and matches the
# original. -Wpointer-sign stays off (126 hits, none changes behaviour; same notes).
set(GAME_FLAGS ${GUEST_CODE_FLAGS} -std=gnu11 -D_GNU_SOURCE -DHALO_LINUX_PLATFORM_LAYER
    -DHALO_ARM64_GUEST=1 -DOPENFRENZY_GUEST=1 -DOPENFRENZY_GAME=1
    -Werror=int-conversion -Werror=incompatible-pointer-types
    -Werror=incompatible-function-pointer-types -Werror=parentheses
    -Wno-pointer-sign -Wno-nonportable-include-path
    "SHELL:-include ${SRC}/runtime/include/linux/halo_linux_prefix.h"
    "SHELL:-include ${GEN}/platform_msvc_semantics.h"
    "SHELL:-include ${SRC}/include/of_game.h"
    ${GAME_INCLUDES} ${PLATFORM_INCLUDES} ${LIBC_SYSTEM_INCLUDES})

# The weak stubs see only of_types.h: their (void) parameter lists differ from the prototypes.
add_library(game_stubs OBJECT "${GEN}/of_stubs.c")
add_dependencies(game_stubs game_decls guest_headers)
target_compile_options(game_stubs PRIVATE ${GUEST_CODE_FLAGS} -std=gnu11 -w -I${SRC}/include
  ${LIBC_SYSTEM_INCLUDES})

add_executable(openfrenzy_guest ${_game_sources} "${GEN}/of_crt_init.c"
  "${SRC}/runtime/of_boot.c" $<TARGET_OBJECTS:game_stubs> $<TARGET_OBJECTS:guest_runtime>)
add_dependencies(openfrenzy_guest guest_runtime game_decls guest_headers)
target_compile_options(openfrenzy_guest PRIVATE ${GAME_FLAGS})
# Whole archive: a weak stand-in in game code must not keep a real platform member out of the link.
target_link_libraries(openfrenzy_guest PRIVATE "-whole-archive" ofplatform "-no-whole-archive" guestc)
set_target_properties(openfrenzy_guest PROPERTIES SUFFIX ".elf" LINK_DEPENDS "${OPENFRENZY_GUEST_LD_SCRIPT}")
