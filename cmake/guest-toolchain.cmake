# Toolchain for guest code: ILP32 AArch64 (arm64_32) rebased into the host's 4 GB arena.
# See docs/ARCHITECTURE.md sections 3 and 4.
#
# Required cache variables (the top-level CMakeLists.txt passes them):
#   OPENFRENZY_LLVM_BIN       LLVM 22 bin directory (clang, opt, llc)
#   OPENFRENZY_LLD            ld.lld
#   OPENFRENZY_REBASE_PLUGIN  the built guest_rebase.dylib
#
# The compiler is port/macos/tools/guest_cc.py. It runs clang -> opt (of-rebase) -> llc ->
# asm_convert.py -> ELF object. The linker is ld.lld with port/macos/guest.ld.
# guest_cc.py adds the pass plugin and port/macos/tools/*.py to each object's depfile, so a
# toolchain change rebuilds every guest C object (no OBJECT_DEPENDS needed here).

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm64_32)

get_filename_component(_of_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(OPENFRENZY_ROOT "${_of_root}" CACHE PATH "openfrenzy source root")

foreach(_var OPENFRENZY_LLVM_BIN OPENFRENZY_LLD OPENFRENZY_REBASE_PLUGIN)
  if(NOT ${_var})
    message(FATAL_ERROR "guest-toolchain.cmake: ${_var} is not set")
  endif()
endforeach()
# try_compile projects need the same values.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  OPENFRENZY_LLVM_BIN OPENFRENZY_LLD OPENFRENZY_REBASE_PLUGIN)

set(_of_cc "${_of_root}/port/macos/tools/guest_cc.py")

# The driver is a script, so CMake cannot identify it by probing. Force the results.
foreach(_lang C CXX ASM)
  set(CMAKE_${_lang}_COMPILER "${_of_cc}")
  set(CMAKE_${_lang}_COMPILER_ID Clang)
  set(CMAKE_${_lang}_COMPILER_VERSION 22.0)
  set(CMAKE_${_lang}_COMPILER_FORCED TRUE)
  set(CMAKE_${_lang}_COMPILER_WORKS TRUE)
endforeach()
set(CMAKE_C_SIZEOF_DATA_PTR 4)
set(CMAKE_CXX_SIZEOF_DATA_PTR 4)
set(CMAKE_C_COMPILER_ABI ELF)
set(CMAKE_CXX_COMPILER_ABI ELF)

# The guest ABI (from OpenCE tools/android_build.py GUEST_ABI_FLAGS).
# -nostdinc is left to each guest project: the skeleton uses clang's freestanding headers,
# the game uses guest musl.
set(OPENFRENZY_GUEST_ABI_FLAGS
  "--of-llvm-bin=${OPENFRENZY_LLVM_BIN}"
  "--of-rebase-plugin=${OPENFRENZY_REBASE_PLUGIN}"
  --target=arm64_32-apple-watchos
  -U__APPLE__ -U__MACH__ -fno-define-target-os-macros
  -D__linux__=1 -D__unix__=1
  -mcpu=cortex-a53
  -fshort-wchar
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
  -femulated-tls
  -mllvm -aarch64-neon-syntax=generic
  -ffp-contract=off
  -O2
)
string(JOIN " " _of_flags ${OPENFRENZY_GUEST_ABI_FLAGS})
set(CMAKE_C_FLAGS_INIT "${_of_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_of_flags} -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT "--of-llvm-bin=${OPENFRENZY_LLVM_BIN}")

# Link: one static ELF image at OPENFRENZY_GUEST_IMAGE_BASE.
set(OPENFRENZY_GUEST_LD_SCRIPT "${_of_root}/port/macos/guest.ld")
set(_of_link "${OPENFRENZY_LLD} -m aarch64linux -static -nostdlib -T ${OPENFRENZY_GUEST_LD_SCRIPT} -Map <TARGET>.map <OBJECTS> <LINK_LIBRARIES> -o <TARGET>")
set(CMAKE_C_LINK_EXECUTABLE "${_of_link}")
set(CMAKE_CXX_LINK_EXECUTABLE "${_of_link}")
set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set(CMAKE_AR "${OPENFRENZY_LLVM_BIN}/llvm-ar")
set(CMAKE_RANLIB "${OPENFRENZY_LLVM_BIN}/llvm-ranlib")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
