# Guest build-skeleton project. Built by the top-level CMakeLists.txt with
# cmake/guest-toolchain.cmake. Produces two guest images:
#   openfrenzy_skeleton.elf  empty main(), exit code 0
#   openfrenzy_probe.elf     rebased memory probe, exit code 42
#   openfrenzy_adrp.elf      global addresses keep a zero upper half (adrp bias), exit code 0
cmake_minimum_required(VERSION 3.24)
project(openfrenzy_guest_skeleton C ASM)

set(OF_MACOS "${OPENFRENZY_ROOT}/port/macos")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Import stubs for the host functions (the host generates its side from the same list).
set(_imports_s "${CMAKE_CURRENT_BINARY_DIR}/imports.s")
add_custom_command(OUTPUT "${_imports_s}"
  COMMAND "${Python3_EXECUTABLE}" -I "${OF_MACOS}/tools/host_imports.py" "${_imports_s}"
          "${OF_MACOS}/host_imports.list"
  DEPENDS "${OF_MACOS}/tools/host_imports.py" "${OF_MACOS}/host_imports.list"
  COMMENT "Guest host-import stubs")

add_library(skeleton_runtime OBJECT crt0.c "${_imports_s}")
target_include_directories(skeleton_runtime PUBLIC "${OF_MACOS}/include")
target_compile_options(skeleton_runtime PUBLIC
  $<$<COMPILE_LANGUAGE:C>:-ffreestanding -DOPENFRENZY_MACOS=1>)

foreach(_name skeleton probe adrp)
  add_executable(openfrenzy_${_name} ${_name}.c)
  # An OBJECT library: the image header must be linked although nothing calls it.
  target_link_libraries(openfrenzy_${_name} PRIVATE skeleton_runtime)
  set_target_properties(openfrenzy_${_name} PROPERTIES SUFFIX ".elf"
    LINK_DEPENDS "${OPENFRENZY_GUEST_LD_SCRIPT}")
endforeach()
