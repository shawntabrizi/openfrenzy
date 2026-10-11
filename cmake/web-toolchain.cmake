# Toolchain for the web build (WebAssembly, docs/WEB.md): Emscripten's own CMake platform file,
# found from emcc on the PATH (Homebrew: brew install emscripten) or from $EMSDK.
#
#   cmake --preset web            # build-web, Ninja
#   cmake --build build-web
#
# The Emscripten cache (system libraries, the SDL3 port) is kept in deps/emscripten-cache, not in
# the Emscripten install: a Homebrew install may be read-only, and deps/ is git-ignored.

get_filename_component(_of_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(OPENFRENZY_EM_CACHE "${_of_root}/deps/emscripten-cache" CACHE PATH "Emscripten cache directory")
set(ENV{EM_CACHE} "${OPENFRENZY_EM_CACHE}")

find_program(OPENFRENZY_EMCC emcc HINTS "$ENV{EMSDK}/upstream/emscripten" /opt/homebrew/bin)
if(NOT OPENFRENZY_EMCC)
  message(FATAL_ERROR "web-toolchain.cmake: emcc not found (brew install emscripten, or source emsdk_env.sh)")
endif()
get_filename_component(_em_bin "${OPENFRENZY_EMCC}" DIRECTORY)
# Emscripten's Python tools need Python 3.10 or later. Its CMake platform file and CMake's
# compile rules call them without Homebrew's wrapper scripts, so name one: EMSDK_PYTHON (emsdk
# sets it), else a python3.1x on the PATH. port/web/CMakeLists.txt passes it to every rule.
if(DEFINED ENV{EMSDK_PYTHON})
  set(OPENFRENZY_EM_PYTHON "$ENV{EMSDK_PYTHON}" CACHE FILEPATH "Python for Emscripten")
else()
  find_program(OPENFRENZY_EM_PYTHON NAMES python3.14 python3.13 python3.12 python3.11 python3.10
               HINTS /opt/homebrew/bin)
endif()
if(OPENFRENZY_EM_PYTHON)
  set(ENV{EMSDK_PYTHON} "${OPENFRENZY_EM_PYTHON}")
endif()
execute_process(COMMAND "${_em_bin}/em-config" EMSCRIPTEN_ROOT
                OUTPUT_VARIABLE _em_root OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
include("${_em_root}/cmake/Modules/Platform/Emscripten.cmake")
