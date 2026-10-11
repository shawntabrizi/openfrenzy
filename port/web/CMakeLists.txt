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
add_executable(openfrenzy_web ${_game_sources} "${GEN}/of_crt_init.c" "${SRC}/runtime/of_boot.c")
add_dependencies(openfrenzy_web web_generated)
target_compile_options(openfrenzy_web PRIVATE ${GAME_FLAGS})
target_link_libraries(openfrenzy_web PRIVATE "-Wl,--whole-archive" ofplatform_web "-Wl,--no-whole-archive" web_runtime
  web_bink ${_ffmpeg_libs})
set(_assertions 0)
if(OPENFRENZY_WEB_ASSERTIONS)
  set(_assertions 1)
endif()
target_link_options(openfrenzy_web PRIVATE
  -O1 -pthread "--use-port=${SDL_PORT}"
  # a direct call whose type does not match its callee is an error, not a trap at run time
  -Wl,--fatal-warnings
  # the game's main on a worker of its own, where it may block; the page's thread passes events
  -sPROXY_TO_PTHREAD=1
  -sPTHREAD_POOL_SIZE=12
  -sOFFSCREENCANVAS_SUPPORT=1
  "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas"
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
  -sGL_ENABLE_GET_PROC_ADDRESS=1
  # one yield to the browser per frame, from inside the game's loops (web_frame.c)
  -sJSPI=1
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
set_target_properties(openfrenzy_web PROPERTIES OUTPUT_NAME openfrenzy SUFFIX ".js"
  RUNTIME_OUTPUT_DIRECTORY "${SITE}" LINK_DEPENDS "${_library_js};${_pre_js};${SDL_PORT}")

# Indirect calls whose declared type is not the callee's are adapted after the link, as
# AArch64 would pass the arguments (port/web/tools/wasm_icall_adapt.py, docs/WEB.md).
set(_binaryen "${OPENFRENZY_EM_ROOT}/binaryen/bin")
if(NOT EXISTS "${_binaryen}/wasm-opt")
  get_filename_component(_binaryen "${OPENFRENZY_EM_ROOT}/../bin" ABSOLUTE)
endif()
if(NOT EXISTS "${_binaryen}/wasm-opt")
  message(FATAL_ERROR "port/web: Binaryen (wasm-opt, wasm-dis) not found next to Emscripten (${OPENFRENZY_EM_ROOT})")
endif()
add_custom_command(TARGET openfrenzy_web POST_BUILD
  COMMAND "${Python3_EXECUTABLE}" -I "${WEB}/tools/wasm_icall_adapt.py" --binaryen "${_binaryen}"
          "--opt=${OPENFRENZY_WEB_WASM_OPT}" --map "${CMAKE_BINARY_DIR}/openfrenzy.icall.txt"
          "${SITE}/openfrenzy.wasm" "${SITE}/openfrenzy.adapted.wasm"
  COMMAND "${CMAKE_COMMAND}" -E rename "${SITE}/openfrenzy.adapted.wasm" "${SITE}/openfrenzy.wasm"
  COMMENT "Adapting indirect calls (wasm_icall_adapt.py)"
  VERBATIM)

# ---------- the page (port/web/site), copied beside openfrenzy.js

file(GLOB _site_files CONFIGURE_DEPENDS "${WEB}/site/*")
set(_site_outputs)
foreach(_f ${_site_files})
  get_filename_component(_name "${_f}" NAME)
  add_custom_command(OUTPUT "${SITE}/${_name}" COMMAND "${CMAKE_COMMAND}" -E copy "${_f}" "${SITE}/${_name}"
    DEPENDS "${_f}")
  list(APPEND _site_outputs "${SITE}/${_name}")
endforeach()
add_custom_target(web_site ALL DEPENDS ${_site_outputs})

# ---------- tests (ctest --test-dir build-web -L web): docs/WEB.md "Tests"

enable_testing()
