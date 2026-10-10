# openfrenzy: native Apple Silicon build (docs/ARCHITECTURE.md).
#
#   cmake -S . -B build -G Ninja
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# This project builds the native parts with Apple clang:
#   - guest_rebase.dylib  the of-rebase LLVM pass (built with LLVM 22's clang++)
#   - openfrenzy          the Darwin host (loader, arena, threads, SDL3, ANGLE)
# Guest code (arm64_32, rebased) is built by sub-projects with cmake/guest-toolchain.cmake.
cmake_minimum_required(VERSION 3.24)
project(openfrenzy C CXX ASM)

if(NOT APPLE OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  message(FATAL_ERROR "openfrenzy builds only on Apple Silicon macOS (arm64)")
endif()

# ---------- tools (Homebrew: llvm@22 lld@22 sdl3 ninja)

set(OPENFRENZY_LLVM_BIN "/opt/homebrew/opt/llvm@22/bin" CACHE PATH "LLVM 22 bin directory")
find_program(OPENFRENZY_LLD ld.lld HINTS /opt/homebrew/opt/lld@22/bin REQUIRED)
execute_process(COMMAND "${OPENFRENZY_LLVM_BIN}/llvm-config" --version
                OUTPUT_VARIABLE _llvm_version OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _llvm_result)
if(NOT _llvm_result EQUAL 0 OR NOT _llvm_version MATCHES "^22\\.")
  message(FATAL_ERROR "The of-rebase pass needs LLVM 22 (brew install llvm@22); "
                      "${OPENFRENZY_LLVM_BIN} gives '${_llvm_version}'")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/sdl3)
find_package(SDL3 REQUIRED CONFIG)
get_target_property(OPENFRENZY_SDL_INCLUDE SDL3::Headers INTERFACE_INCLUDE_DIRECTORIES)
list(GET OPENFRENZY_SDL_INCLUDE 0 OPENFRENZY_SDL_INCLUDE)
include(cmake/Dependencies.cmake)

set(OF_MACOS "${CMAKE_SOURCE_DIR}/port/macos")

# ---------- the of-rebase compiler pass

execute_process(COMMAND "${OPENFRENZY_LLVM_BIN}/llvm-config" --cxxflags --ldflags --libs core passes
                OUTPUT_VARIABLE _llvm_flags OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
separate_arguments(_llvm_flags UNIX_COMMAND "${_llvm_flags}")
set(OPENFRENZY_REBASE_PLUGIN "${CMAKE_BINARY_DIR}/guest_rebase.dylib")
add_custom_command(OUTPUT "${OPENFRENZY_REBASE_PLUGIN}"
  COMMAND "${OPENFRENZY_LLVM_BIN}/clang++" -shared -fPIC "${OF_MACOS}/compiler/guest_rebase.cpp"
          -o "${OPENFRENZY_REBASE_PLUGIN}" ${_llvm_flags}
  DEPENDS "${OF_MACOS}/compiler/guest_rebase.cpp"
  COMMENT "Building the of-rebase LLVM pass")
add_custom_target(guest_rebase_plugin ALL DEPENDS "${OPENFRENZY_REBASE_PLUGIN}")

# ---------- the Darwin host

# posix_files.c implements src/vfs/posix.h, the platform layer's file interface.
set(OPENFRENZY_POSIX_INCLUDE "${CMAKE_SOURCE_DIR}/src/vfs" CACHE PATH "Directory with posix.h")
if(NOT EXISTS "${OPENFRENZY_POSIX_INCLUDE}/posix.h")
  message(FATAL_ERROR "posix.h not found in ${OPENFRENZY_POSIX_INCLUDE} (platform layer)")
endif()

# The host table covers host_imports.list and the posix_* functions (the guest calls them as
# hostposix_*). GL imports are resolved at load time (host_gl_resolve).
set(_host_table "${CMAKE_BINARY_DIR}/host/host_import_table.c")
set(_posix_list "${CMAKE_BINARY_DIR}/host/posix_imports.list")
add_custom_command(OUTPUT "${_posix_list}"
  COMMAND "${Python3_EXECUTABLE}" -I "${OF_MACOS}/tools/posix_stubs.py"
          "${OPENFRENZY_POSIX_INCLUDE}/posix.h" "${CMAKE_BINARY_DIR}/host/unused_posix.c" "${_posix_list}"
  DEPENDS "${OF_MACOS}/tools/posix_stubs.py" "${OPENFRENZY_POSIX_INCLUDE}/posix.h")
add_custom_command(OUTPUT "${_host_table}"
  COMMAND "${Python3_EXECUTABLE}" -I "${OF_MACOS}/tools/host_imports.py"
          --host-table "${_host_table}" "${CMAKE_BINARY_DIR}/host/unused_imports.s"
          "${OF_MACOS}/host_imports.list" "${_posix_list}"
  DEPENDS "${OF_MACOS}/tools/host_imports.py" "${OF_MACOS}/host_imports.list" "${_posix_list}"
  COMMENT "Host import table")

file(GLOB _host_sources CONFIGURE_DEPENDS "${OF_MACOS}/host/*.c")

add_executable(openfrenzy ${_host_sources} "${OF_MACOS}/host/entry.s" "${_host_table}")
target_compile_definitions(openfrenzy PRIVATE OPENFRENZY_MACOS=1 _DARWIN_C_SOURCE)
target_compile_options(openfrenzy PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-O2 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter>)
target_include_directories(openfrenzy PRIVATE
  "${CMAKE_SOURCE_DIR}" "${OF_MACOS}/host" "${OF_MACOS}/include"
  "${OPENFRENZY_POSIX_INCLUDE}" "${OPENFRENZY_GL_INCLUDE}")
target_link_libraries(openfrenzy PRIVATE SDL3::SDL3)
target_link_options(openfrenzy PRIVATE
  "-F${OPENFRENZY_EGL_FRAMEWORKS}" "-F${OPENFRENZY_GLES_FRAMEWORKS}"
  "SHELL:-framework libEGL" "SHELL:-framework libGLESv2"
  "-Wl,-rpath,${OPENFRENZY_EGL_FRAMEWORKS}" "-Wl,-rpath,${OPENFRENZY_GLES_FRAMEWORKS}")
set_target_properties(openfrenzy PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# ---------- guest images

include(ExternalProject)
set(_guest_args
  "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/guest-toolchain.cmake"
  "-DOPENFRENZY_LLVM_BIN=${OPENFRENZY_LLVM_BIN}"
  "-DOPENFRENZY_LLD=${OPENFRENZY_LLD}"
  "-DOPENFRENZY_REBASE_PLUGIN=${OPENFRENZY_REBASE_PLUGIN}"
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}")

# Build skeleton: an empty main() and a memory probe (port/macos/tests/skeleton).
ExternalProject_Add(guest_skeleton
  SOURCE_DIR "${OF_MACOS}/tests/skeleton"
  BINARY_DIR "${CMAKE_BINARY_DIR}/guest-skeleton"
  CMAKE_ARGS ${_guest_args}
  DEPENDS guest_rebase_plugin
  BUILD_ALWAYS ON
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/openfrenzy_skeleton.elf"
                   "${CMAKE_BINARY_DIR}/openfrenzy_probe.elf")

# Platform layer, smoke test and the game (port/guest): musl, guest runtime, src/.
ExternalProject_Add(guest_main
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/port/guest"
  BINARY_DIR "${CMAKE_BINARY_DIR}/guest-main"
  CMAKE_ARGS ${_guest_args}
    "-DOPENFRENZY_MUSL_DIR=${OPENFRENZY_MUSL_DIR}"
    "-DOPENFRENZY_MUSL_VERSION=${OPENFRENZY_MUSL_VERSION}"
    "-DOPENFRENZY_GL_INCLUDE=${OPENFRENZY_GL_INCLUDE}"
    "-DOPENFRENZY_SDL_INCLUDE=${OPENFRENZY_SDL_INCLUDE}"
  CMAKE_GENERATOR Ninja
  DEPENDS guest_rebase_plugin
  BUILD_ALWAYS ON
  USES_TERMINAL_BUILD ON
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/openfrenzy_smoke.elf" "${CMAKE_BINARY_DIR}/openfrenzy_guest.elf")

# ---------- tests: run guest images in the host arena

enable_testing()
foreach(_case "skeleton;0" "probe;42" "platform_check;0" "guest;0")
  list(GET _case 0 _name)
  list(GET _case 1 _code)
  add_test(NAME guest_${_name}_runs
    COMMAND "${CMAKE_COMMAND}"
      "-DHOST=$<TARGET_FILE:openfrenzy>"
      "-DIMAGE=${CMAKE_BINARY_DIR}/openfrenzy_${_name}.elf"
      "-DWORK=${CMAKE_BINARY_DIR}/test-${_name}"
      "-DEXPECT=${_code}"
      -P "${CMAKE_SOURCE_DIR}/cmake/run_guest_test.cmake")
endforeach()

# Smoke test of the full guest stack (musl, runtime, imports, vfs disc reader). Needs the
# player's disc image; without it the test is not registered.
set(OPENFRENZY_ISO "${CMAKE_SOURCE_DIR}/orig/game.xiso.iso" CACHE FILEPATH "Game disc image (XDVDFS)")
if(EXISTS "${OPENFRENZY_ISO}")
  add_test(NAME guest_smoke_runs
    COMMAND "${CMAKE_COMMAND}"
      "-DHOST=$<TARGET_FILE:openfrenzy>"
      "-DIMAGE=${CMAKE_BINARY_DIR}/openfrenzy_smoke.elf"
      "-DWORK=${CMAKE_BINARY_DIR}/test-smoke"
      "-DEXPECT=0"
      "-DISO=${OPENFRENZY_ISO}"
      -P "${CMAKE_SOURCE_DIR}/cmake/run_guest_test.cmake")
else()
  message(STATUS "No disc image at ${OPENFRENZY_ISO}: guest_smoke_runs test not registered")
endif()
