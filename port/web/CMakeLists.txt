# The web build: the game as WebAssembly in a browser (docs/WEB.md, docs/ARCHITECTURE.md
# section 18). Configured from the top-level CMakeLists.txt with cmake/web-toolchain.cmake
# (cmake --preset web). Output: build-web/site/ (the page, openfrenzy.js, openfrenzy.wasm).
#
# Design after OpenCE's web port (port/web, tools/web_build.py at e0b581b1, CC0):
#   - wasm32 is ILP32 like the Xbox: the game is compiled directly, with no guest/host split
#     and no of-rebase pass (those exist because macOS has no ILP32 process).
#   - Emscripten's C library (musl-based) instead of the guest musl; Emscripten pthreads;
#     WebGL 2 through Emscripten's GLES 3; SDL 3.4.16 as an Emscripten port.
#   - PROXY_TO_PTHREAD: the game's main runs on a worker and may block. Its canvas is moved
#     to that worker (OffscreenCanvas). The game yields to the browser once a frame at Present
#     with JSPI (port/web/src/web_frame.c), so the game's own loops stay as they are.
#   - The memory is 0x88000000 bytes: the Xbox contiguous window 0x80000000-0x88000000 is its
#     top 128 MB, the C heap stays below it (web_main.c).
cmake_minimum_required(VERSION 3.24)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(R "${CMAKE_SOURCE_DIR}")
set(SRC "${R}/src")
set(WEB "${R}/port/web")
set(GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(SITE "${CMAKE_BINARY_DIR}/site")
file(MAKE_DIRECTORY "${GEN}" "${SITE}")

# Every compile and link runs with the Emscripten cache in deps/ and a Python that Emscripten
# accepts (cmake/web-toolchain.cmake): emcc reads both only from its environment.
set(_em_env "${CMAKE_COMMAND}" -E env "EM_CACHE=${OPENFRENZY_EM_CACHE}")
if(OPENFRENZY_EM_PYTHON)
  list(APPEND _em_env "EMSDK_PYTHON=${OPENFRENZY_EM_PYTHON}")
endif()
list(JOIN _em_env " " _em_env)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${_em_env}")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${_em_env}")
# the optimisation level is the one below, for every build type
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_DEBUG "")

option(OPENFRENZY_WEB_ASSERTIONS "Emscripten run-time assertions (slower)" OFF)
# Binaryen's optimisation of the linked module: -O2, or empty to keep every function apart in
# stack traces. Emscripten links at -O1, which does not run Binaryen: port/web/tools/
# wasm_icall_adapt.py must see the module before any inlining (it gives bodies to LLVM's
# trapping "_bitcast_invalid" stubs, which -O2 would inline into their callers), and then
# runs wasm-opt with this level itself.
set(OPENFRENZY_WEB_WASM_OPT "-O2" CACHE STRING "wasm-opt level of the web build after the call adapter")

set(OPENFRENZY_PLATFORM_MODULES runtime vfs xkernel d3d8 dsound input)
set(SDL_PORT "${WEB}/openfrenzy_sdl3.py")

# ---------- flags

# The ABI the game was written for, as the native guest has it (cmake/guest-toolchain.cmake):
# 16-bit wchar_t, no fused multiply-adds. wasm32 aligns doubles and 64-bit integers to 8.
set(WEB_ABI_FLAGS -fshort-wchar -ffp-contract=off -pthread -msimd128 -mbulk-memory
    -mnontrapping-fptoint -O2 "--use-port=${SDL_PORT}")
set(WEB_CODE_FLAGS -fms-extensions -fcommon -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks)
# Emscripten's C library has a 32-bit wchar_t: no loop may turn into a call to it
foreach(_n wcslen wcsnlen wcschr wcsrchr wcscmp wcsncmp wcscpy wcsncpy wcscat wcsncat wmemchr wmemcmp
           wmemcpy wmemmove wmemset)
  list(APPEND WEB_CODE_FLAGS -fno-builtin-${_n})
endforeach()
set(WEB_DEFINES -D_GNU_SOURCE -DHALO_LINUX_PLATFORM_LAYER -DHALO_GLES=1 -DHALO_WEB=1
    -DOPENFRENZY_GUEST=1 -DOPENFRENZY_WEB=1)

# ---------- generated sources (the same generators as the native build)

add_custom_command(OUTPUT "${GEN}/platform_msvc_semantics.h"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/scripts/linux_msvc_semantics.py"
          --output "${GEN}/platform_msvc_semantics.h" --inlines "${SRC}/runtime/include/xdk"
  DEPENDS "${R}/scripts/linux_msvc_semantics.py")
set(_smaa "${R}/port/third_party/smaa")
add_custom_command(OUTPUT "${GEN}/xgpu_smaa.c"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/embed_smaa.py" "${_smaa}" "${GEN}/xgpu_smaa.c"
  DEPENDS "${R}/port/macos/tools/embed_smaa.py" "${_smaa}/SMAA.hlsl" "${_smaa}/area_tex.zlib"
          "${_smaa}/search_tex.zlib")
file(GLOB_RECURSE _decl_files CONFIGURE_DEPENDS "${SRC}/*.decls")
set(_table "${SRC}/include/of_functions.tsv")
add_custom_command(OUTPUT "${GEN}/of_decls.h" "${GEN}/of_stubs.c" "${GEN}/of_crt_init.c"
  COMMAND "${Python3_EXECUTABLE}" -I "${R}/port/macos/tools/gen_game_decls.py"
          --table "${_table}" --src "${SRC}" --out "${GEN}"
  DEPENDS "${R}/port/macos/tools/gen_game_decls.py" "${_table}" ${_decl_files} "${SRC}/include/of_types.h"
  COMMENT "Game declarations and weak stubs")
add_custom_target(web_generated DEPENDS "${GEN}/platform_msvc_semantics.h" "${GEN}/of_decls.h")

# ---------- the platform layer (XDK replacement) and the web runtime

set(PLATFORM_INCLUDES)
foreach(_m ${OPENFRENZY_PLATFORM_MODULES})
  list(APPEND PLATFORM_INCLUDES -I${SRC}/${_m})
endforeach()
list(APPEND PLATFORM_INCLUDES -I${SRC}/runtime/compat -I${SRC}/runtime/include/linux
     -I${SRC}/runtime/include -I${SRC}/runtime/include/xdk -I${SRC}/runtime/entry -I${WEB}/src
     -I${R}/port/guest/runtime -I${R}/port/macos/include)
set(PLATFORM_FLAGS ${WEB_ABI_FLAGS} ${WEB_CODE_FLAGS} -std=gnu11 ${WEB_DEFINES} -w
    "SHELL:-include ${SRC}/runtime/include/linux/halo_linux_prefix.h"
    "SHELL:-include ${GEN}/platform_msvc_semantics.h"
    ${PLATFORM_INCLUDES})

set(_platform_sources)
foreach(_m ${OPENFRENZY_PLATFORM_MODULES})
  file(GLOB _f CONFIGURE_DEPENDS "${SRC}/${_m}/*.c" "${SRC}/${_m}/compat/*.c" "${SRC}/${_m}/entry/*.c")
  list(APPEND _platform_sources ${_f})
endforeach()
list(FILTER _platform_sources EXCLUDE REGEX "/src/runtime/(of_boot|of_main)\\.c$")
list(APPEND _platform_sources "${GEN}/xgpu_smaa.c"
     "${WEB}/src/web_gl_host.c" "${WEB}/src/web_memory_watch.c" "${WEB}/src/web_frame.c"
     "${WEB}/src/web_icall.c")
add_library(ofplatform_web STATIC ${_platform_sources})
add_dependencies(ofplatform_web web_generated)
target_compile_options(ofplatform_web PRIVATE ${PLATFORM_FLAGS})

# Bink videos: port/macos/host/host_bink.c as it is (web_bink.c), on an LGPL FFmpeg built for wasm
# (port/web/ffmpeg.cmake). Off: every BinkOpen fails and the game skips its videos.
option(OPENFRENZY_WEB_BINK_FFMPEG "Play Bink videos with FFmpeg built for WebAssembly" ON)
set(_bink_ffmpeg 0)
set(_ffmpeg_libs)
if(OPENFRENZY_WEB_BINK_FFMPEG)
  include("${WEB}/ffmpeg.cmake")
  openfrenzy_web_ffmpeg()
  set(_bink_ffmpeg 1)
  foreach(_lib avformat avcodec swscale avutil)
    list(APPEND _ffmpeg_libs "${OPENFRENZY_WEB_FFMPEG_DIR}/lib/lib${_lib}.a")
  endforeach()
  message(STATUS "Bink video (web): FFmpeg in ${OPENFRENZY_WEB_FFMPEG_DIR}")
endif()
add_library(web_bink STATIC "${WEB}/src/web_bink.c")
target_compile_options(web_bink PRIVATE ${WEB_ABI_FLAGS} -std=gnu11 -D_GNU_SOURCE -w
  -DOPENFRENZY_BINK_FFMPEG=${_bink_ffmpeg})
if(OPENFRENZY_WEB_BINK_FFMPEG)
  target_include_directories(web_bink PRIVATE "${OPENFRENZY_WEB_FFMPEG_DIR}/include")
endif()

# the C library's side (posix.h, the page's entry point): Emscripten's POSIX only
add_library(web_runtime STATIC "${WEB}/src/posix_files.c" "${WEB}/src/web_main.c" "${WEB}/src/host_watch_hash.c")
target_compile_options(web_runtime PRIVATE ${WEB_ABI_FLAGS} -std=gnu11 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -w
  -I${SRC}/vfs -I${WEB}/src)

# ---------- the game: every other src/<module>/ + generated declarations and weak stubs

file(GLOB _src_dirs LIST_DIRECTORIES true RELATIVE "${SRC}" "${SRC}/*")
set(_game_sources)
set(GAME_INCLUDES -I${SRC}/include -I${GEN})
foreach(_d ${_src_dirs})
  if(IS_DIRECTORY "${SRC}/${_d}" AND NOT _d IN_LIST OPENFRENZY_PLATFORM_MODULES AND NOT _d STREQUAL "include")
    file(GLOB_RECURSE _f CONFIGURE_DEPENDS "${SRC}/${_d}/*.c")
    list(FILTER _f EXCLUDE REGEX "/tests/")
    list(APPEND _game_sources ${_f})
    list(APPEND GAME_INCLUDES -I${SRC}/${_d})
  endif()
endforeach()
# the same warnings-as-errors as the native game build (port/guest/CMakeLists.txt)
set(GAME_FLAGS ${WEB_ABI_FLAGS} ${WEB_CODE_FLAGS} -std=gnu11 ${WEB_DEFINES} -DOPENFRENZY_GAME=1
    -Werror=int-conversion -Werror=incompatible-pointer-types
    -Werror=incompatible-function-pointer-types -Werror=parentheses
    -Wno-pointer-sign -Wno-nonportable-include-path
    "SHELL:-include ${SRC}/runtime/include/linux/halo_linux_prefix.h"
    "SHELL:-include ${GEN}/platform_msvc_semantics.h"
    "SHELL:-include ${SRC}/include/of_game.h"
    ${GAME_INCLUDES} ${PLATFORM_INCLUDES})

# No weak stubs (gen/of_stubs.c): every function of the table has a definition (the native
# image has no weak FUN_ symbol left, docs/STATUS.md). Without them a missing function is a
# link error, and wasm-ld's check of every direct call's type is not drowned in 5000 warnings
# about the stubs' (void) parameter lists.

# File-scope assembly in the game's data files is written for the native toolchain (Mach-O
# symbol names, converted by port/macos/tools/asm_convert.py). The web build compiles a copy
# with the same conversion applied to the assembly strings (port/web/tools/wasm_inline_asm.py).
set(_converted_sources)
foreach(_f ${_game_sources})
  file(STRINGS "${_f}" _asm_lines REGEX "(__asm__|[^_a-zA-Z]asm) *\\(" LIMIT_COUNT 1)
  if(_asm_lines)
    file(RELATIVE_PATH _rel "${SRC}" "${_f}")
    set(_out "${GEN}/asm/${_rel}")
    get_filename_component(_dir "${_f}" DIRECTORY)
    add_custom_command(OUTPUT "${_out}"
      COMMAND "${Python3_EXECUTABLE}" -I "${WEB}/tools/wasm_inline_asm.py" "${_f}" "${_out}"
      DEPENDS "${_f}" "${WEB}/tools/wasm_inline_asm.py")
    set_source_files_properties("${_out}" PROPERTIES COMPILE_OPTIONS "-iquote;${_dir}")
    list(APPEND _converted_sources "${_out}")
  else()
    list(APPEND _converted_sources "${_f}")
  endif()
endforeach()
set(_game_sources ${_converted_sources})

# the game's main() becomes of_game_main: web_main.c is the program's main
set_source_files_properties("${SRC}/runtime/of_boot.c" PROPERTIES COMPILE_DEFINITIONS "main=of_game_main")

set(_library_js "${WEB}/src/web_library.js")
set(_pre_js "${WEB}/src/web_pre.js")
# the game's objects, linked twice when OPENFRENZY_WEB_ASYNCIFY is on
add_library(game_web_objects OBJECT ${_game_sources} "${GEN}/of_crt_init.c" "${SRC}/runtime/of_boot.c")
add_dependencies(game_web_objects web_generated)
target_compile_options(game_web_objects PRIVATE ${GAME_FLAGS})

set(_assertions 0)
if(OPENFRENZY_WEB_ASSERTIONS)
  set(_assertions 1)
endif()
set(_link_common
  -pthread "--use-port=${SDL_PORT}"
  # a direct call whose type does not match its callee is an error, not a trap at run time
  -Wl,--fatal-warnings
  # the game's main on a worker of its own, where it may block; the page's thread passes events
  -sPROXY_TO_PTHREAD=1
  -sPTHREAD_POOL_SIZE=12
  -sOFFSCREENCANVAS_SUPPORT=1
  "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas"
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
  -sGL_ENABLE_GET_PROC_ADDRESS=1
  # the page's storage (OPFS) at /data: the disc image, saves, settings
  -sWASMFS=1 -lopfs.js
  # the Xbox contiguous window is the top 128 MB (web_main.c)
  -sINITIAL_MEMORY=2281701376 -sMAXIMUM_MEMORY=2281701376 -sALLOW_MEMORY_GROWTH=0
  -sSTACK_SIZE=16MB -sDEFAULT_PTHREAD_STACK_SIZE=1MB
  -sEXIT_RUNTIME=0
  -sENVIRONMENT=web,worker
  -sMODULARIZE=1 -sEXPORT_NAME=createOpenFrenzy
  -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPU32
  -sEXPORTED_FUNCTIONS=_main,_malloc,_free
  -sASSERTIONS=${_assertions}
  # function names in stack traces (the name section only)
  --profiling-funcs
  "SHELL:--js-library ${_library_js}"
  "SHELL:--pre-js ${_pre_js}")

function(openfrenzy_web_executable target output)
  add_executable(${target} $<TARGET_OBJECTS:game_web_objects>)
  target_link_libraries(${target} PRIVATE "-Wl,--whole-archive" ofplatform_web "-Wl,--no-whole-archive" web_runtime
    web_bink ${_ffmpeg_libs})
  target_link_options(${target} PRIVATE ${_link_common} ${ARGN})
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output} SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${SITE}" LINK_DEPENDS "${_library_js};${_pre_js};${SDL_PORT}")
endfunction()

# The main build: one yield to the browser per frame with JSPI, from inside the game's loops
# (web_frame.c). Emscripten links at -O1 (no Binaryen): the call adapter below needs the module
# before any inlining, and runs wasm-opt itself.
openfrenzy_web_executable(openfrenzy_web openfrenzy -O1 -sJSPI=1)
# (a change of the adapter relinks, so that its post-build step runs again)
set_property(TARGET openfrenzy_web APPEND PROPERTY LINK_DEPENDS "${WEB}/tools/wasm_icall_adapt.py")

# Indirect calls whose declared type is not the callee's are adapted after the link, as
# AArch64 would pass the arguments (port/web/tools/wasm_icall_adapt.py, docs/WEB.md).
set(_binaryen "${OPENFRENZY_EM_ROOT}/binaryen/bin")
if(NOT EXISTS "${_binaryen}/wasm-opt")
  get_filename_component(_binaryen "${OPENFRENZY_EM_ROOT}/../bin" ABSOLUTE)
endif()
if(NOT EXISTS "${_binaryen}/wasm-opt")
  message(FATAL_ERROR "port/web: Binaryen (wasm-opt, wasm-dis) not found next to Emscripten (${OPENFRENZY_EM_ROOT})")
endif()
# (no LLVM trapping stub may be left for the Asyncify build, which cannot use the adapter)
set(_no_stubs)
if(OPENFRENZY_WEB_ASYNCIFY)
  set(_no_stubs --no-stubs)
endif()
add_custom_command(TARGET openfrenzy_web POST_BUILD
  COMMAND "${Python3_EXECUTABLE}" -I "${WEB}/tools/wasm_icall_adapt.py" --binaryen "${_binaryen}"
          "--opt=${OPENFRENZY_WEB_WASM_OPT}" --map "${CMAKE_BINARY_DIR}/openfrenzy.icall.txt" ${_no_stubs}
          "${SITE}/openfrenzy.wasm" "${SITE}/openfrenzy.adapted.wasm"
  COMMAND "${CMAKE_COMMAND}" -E rename "${SITE}/openfrenzy.adapted.wasm" "${SITE}/openfrenzy.wasm"
  COMMENT "Adapting indirect calls (wasm_icall_adapt.py)"
  VERBATIM)

# For browsers without JSPI (Safari 26): the same objects with Asyncify for the yield and
# Emscripten's function pointer cast emulation for the mismatched calls (the adapter's br_table
# thunks could not be unwound by Asyncify, which instruments the module first).
option(OPENFRENZY_WEB_ASYNCIFY "Also build openfrenzy-asyncify.js for browsers without JSPI" OFF)
if(OPENFRENZY_WEB_ASYNCIFY)
  # FFmpeg and SDL are never on the stack at the yield (web_frame.c is called from the game's
  # Present, outside them), so they need no Asyncify instrumentation
  openfrenzy_web_executable(openfrenzy_web_asyncify openfrenzy-asyncify -O2 -sASYNCIFY=1
    -sEMULATE_FUNCTION_POINTER_CASTS=1 -sASYNCIFY_STACK_SIZE=1048576
    "-sASYNCIFY_REMOVE=['ff_*','av_*','avcodec_*','avformat_*','avio_*','sws_*','SDL_*']")
  add_dependencies(openfrenzy_web_asyncify openfrenzy_web)
endif()

# ---------- the page (port/web/site), copied beside openfrenzy.js

file(GLOB _site_files CONFIGURE_DEPENDS "${WEB}/site/*")
# the service worker that isolates the page where the host sends no COOP/COEP (GitHub Pages)
set(_coi "${R}/port/third_party/coi-serviceworker")
list(APPEND _site_files "${_coi}/coi-serviceworker.js")
set(_site_outputs)
foreach(_f ${_site_files})
  get_filename_component(_name "${_f}" NAME)
  add_custom_command(OUTPUT "${SITE}/${_name}" COMMAND "${CMAKE_COMMAND}" -E copy "${_f}" "${SITE}/${_name}"
    DEPENDS "${_f}")
  list(APPEND _site_outputs "${SITE}/${_name}")
endforeach()
# licenses.txt: the licences of the site's code (FFmpeg's LGPL notice with its source and
# configure line, SMAA, coi-serviceworker, SDL, Emscripten's runtime and C library, CC0)
set(_licence_head "${GEN}/licenses-head.txt")
file(WRITE "${_licence_head}" "OpenFrenzy web build: licences

OpenFrenzy's own code and OpenCE's are released under CC0 1.0 (the last text below). Source:
https://github.com/shawntabrizi/openfrenzy

openfrenzy.wasm and openfrenzy-asyncify.wasm also contain: FFmpeg (LGPL 2.1 or later; its
notice, the exact source archive and the configure line follow), SMAA (MIT), SDL 3 (zlib
licence) and Emscripten's runtime and C library (MIT / University of Illinois NCSA; its musl
part: MIT). coi-serviceworker.js is coi-serviceworker 0.1.7 (MIT). The game's data is not part
of this site: the player supplies a legally obtained copy.

")
set(_licence_parts "${_licence_head}")
if(OPENFRENZY_WEB_BINK_FFMPEG)
  list(APPEND _licence_parts "${OPENFRENZY_WEB_FFMPEG_DIR}/LICENSES/SOURCE.txt"
       "${OPENFRENZY_WEB_FFMPEG_DIR}/LICENSES/COPYING.LGPLv2.1")
endif()
list(APPEND _licence_parts "${R}/port/third_party/smaa/LICENSE" "${_coi}/LICENSE")
# SDL's licence is in the port's source, which the first compile fetches (openfrenzy_sdl3.py):
# not there yet at configure time in a new cache, so read after the game is built
file(STRINGS "${SDL_PORT}" _sdl_version REGEX "^VERSION = ")
string(REGEX REPLACE "^VERSION = \"([^\"]+)\"$" "\\1" _sdl_version "${_sdl_version}")
set(_sdl_licence "${OPENFRENZY_EM_CACHE}/ports/openfrenzy_sdl3/SDL-release-${_sdl_version}/LICENSE.txt")
# (emsdk has Emscripten's LICENSE in its root; Homebrew one level up)
set(_em_licence "${OPENFRENZY_EM_ROOT}/LICENSE")
if(NOT EXISTS "${_em_licence}")
  get_filename_component(_em_licence "${OPENFRENZY_EM_ROOT}/../LICENSE" ABSOLUTE)
endif()
if(NOT EXISTS "${_em_licence}")
  message(FATAL_ERROR "port/web: the Emscripten licence file is missing (${_em_licence})")
endif()
add_custom_command(OUTPUT "${SITE}/licenses.txt"
  COMMAND "${CMAKE_COMMAND}" -E cat ${_licence_parts} "${_sdl_licence}" "${_em_licence}" "${R}/LICENSE.md"
          > "${SITE}/licenses.txt"
  DEPENDS ${_licence_parts} "${_em_licence}" "${R}/LICENSE.md" VERBATIM)
list(APPEND _site_outputs "${SITE}/licenses.txt")
add_custom_target(web_site ALL DEPENDS ${_site_outputs})
# (after the link, which fetched SDL's source)
add_dependencies(web_site openfrenzy_web)

# ---------- tests (ctest --test-dir build-web -L web): docs/WEB.md "Tests"
#
# Each test serves build-web/site on 127.0.0.1, runs the game in the installed Chrome
# (headless, through Playwright in tools/web-test, git-ignored) with a profile of its own in
# the build directory, and checks the game's log (port/web/tests/drive.mjs). The first test
# copies the player's disc image into that profile's storage. Without Playwright or the disc
# image the tests are skipped (exit 77). They share the profile and the port: RESOURCE_LOCK.

enable_testing()
find_program(OPENFRENZY_NODE node)
set(OPENFRENZY_ISO "${CMAKE_SOURCE_DIR}/orig/game.xiso.iso" CACHE FILEPATH "Game disc image (the player's own)")
if(OPENFRENZY_NODE)
  set(_drive "${OPENFRENZY_NODE}" "${WEB}/tests/drive.mjs" --site "${SITE}" --iso "${OPENFRENZY_ISO}"
    --profile "${CMAKE_BINARY_DIR}/test-profile" --port 8791
    --set OPENFRENZY_BKLOG=1 --set OPENFRENZY_WEB_FRAME_LOG=1
    --forbid "RuntimeError" --forbid "worker sent an error" --forbid "pageerror"
    --forbid "OpenFrenzy fault" --forbid "UNIMPLEMENTED" --forbid "Out of memory" --forbid "[Aa]ssert")
  # boot with the Bink videos: the root mode, the two logo videos to their last frame, sound
  # (the title follows the attract video; web_board reaches it with the videos off)
  add_test(NAME web_boot COMMAND ${_drive} --out "${CMAKE_BINARY_DIR}/test-web/boot" --seconds 30 --shots 10
    --expect "Top of stack now: Root" --expect "bink: closed after 105 of 105 frames"
    --expect "bink: closed after 172 of 172 frames" --expect "audio: peak 0\\.[0-9]*[1-9]")
  # the native board recipe (tests/run_game_check.py) as key events: menus, character select,
  # the Map, a minigame to its countdown; 60 fps for 90 % of the seconds (55 or more)
  set(_keys "6:Enter,9:Space,11:Space,13:Space,16:Enter,20:Enter,24:Enter,28:Enter,32:Space,36:Enter")
  foreach(_t RANGE 40 118 3)
    string(APPEND _keys ",${_t}:Space")
  endforeach()
  add_test(NAME web_board COMMAND ${_drive} --out "${CMAKE_BINARY_DIR}/test-web/board" --seconds 120 --shots 60,119
    --set OPENFRENZY_DEBUG_SKIP_VIDEOS=true --keys "${_keys}" --min-fps 55
    --expect "Top of stack now: CharacterSelect" --expect "Top of stack now: Map" --expect "countdown PLAYING")
  # a scripted Gamepad API pad (standard mapping) through SDL: START and A to character select
  add_test(NAME web_gamepad COMMAND ${_drive} --out "${CMAKE_BINARY_DIR}/test-web/gamepad" --seconds 25
    --set OPENFRENZY_DEBUG_SKIP_VIDEOS=true --pad "6:9,9:0,11:0,13:0" --expect "Top of stack now: CharacterSelect")
  # four scripted pads at once, each a human player: pads 1-3 join at character select and pick
  # (pad 0 has player 1); START, the Map, A to a minigame. From its first "countdown 3" (the
  # mark) each of pads 1-3 is unplugged and plugged in again: the game pauses only for a
  # human player's pad ("Paused!", "Controller N has been disconnected"); A on pad 0 must not
  # end that pause, A on the pad itself does. Then pad 3's d-pad reaches port 3 in the game.
  # With pad 3 left out (--pads 3) or not joined (a CPU player 4) the test fails.
  set(_pads4 "6:9,9:0,11:0,13:0,20:1/0,20.5:2/0,21:3/0,23:1/15,23:2/15,23:3/14,26:0,26.5:1/0,27:2/0,27.5:3/0,31:9,35:9")
  foreach(_t RANGE 38 64 2)
    string(APPEND _pads4 ",${_t}:0")
  endforeach()
  set(_pads4_expect)
  foreach(_p 1 2 3)
    list(APPEND _pads4_expect --expect "driver: pad ${_p} unplug" --expect "driver: pad ${_p} plug"
      --expect " Paused!$" --expect "driver: pad 0 button 0" --expect "driver: pad ${_p} button 0" --expect "End pause")
  endforeach()
  string(APPEND _pads4 ",m6:1/unplug,m7.5:1/plug,m8.5:0,m9.5:1/0,m10.5:2/unplug,m12:2/plug,m13:0,m14:2/0"
    ",m15:3/unplug,m16.5:3/plug,m17.5:0,m18.5:3/0,m19.5:3/14+2500")
  add_test(NAME web_gamepad_4 COMMAND ${_drive} --out "${CMAKE_BINARY_DIR}/test-web/gamepad4" --seconds 100
    --set OPENFRENZY_DEBUG_SKIP_VIDEOS=true --set OPENFRENZY_DEBUG_INPUT_LOG=true
    --pads 4 --pad "${_pads4}" --mark " countdown 3$" --shots 30,m8,m17,m19.3,m22
    --expect "bInitInput found 4 connected input devices" --expect "Top of stack now: CharacterSelect"
    --expect "driver: mark" ${_pads4_expect} --expect "xinput: port 3 packet [0-9]+: buttons 0x0004")
  set_tests_properties(web_boot web_board web_gamepad web_gamepad_4 PROPERTIES LABELS web SKIP_RETURN_CODE 77
    RESOURCE_LOCK web_browser TIMEOUT 400)
  # the call adapter's AArch64 argument rules and the inline assembly converter, on small
  # modules run in Node (no browser, no game data)
  add_test(NAME web_tools COMMAND "${Python3_EXECUTABLE}" -I "${WEB}/tests/test_web_tools.py"
    --binaryen "${_binaryen}" --node "${OPENFRENZY_NODE}")
  set_tests_properties(web_tools PROPERTIES LABELS web TIMEOUT 120)
else()
  message(STATUS "port/web: node not found: no web tests (docs/WEB.md \"Tests\")")
endif()
