# Prebuilt ANGLE (GLES 3 on Metal) and the Khronos EGL/GLES headers, pinned by SHA-256 in
# port/macos/dependencies.json (copied from ref/OpenCE-mac-zimm3rmann, CC0).
# ANGLE has no Homebrew formula, so it comes from this pinned download (docs/ARCHITECTURE.md 6).
# Downloads go to OPENFRENZY_DEPS_DIR (default <source>/deps, git-ignored), shared by all
# build directories. Nothing here downloads game data.

set(OPENFRENZY_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps" CACHE PATH "Downloaded build dependencies")
file(READ "${CMAKE_SOURCE_DIR}/port/macos/dependencies.json" _of_manifest)

function(_of_download url sha256 destination)
  if(EXISTS "${destination}")
    file(SHA256 "${destination}" _have)
    if(_have STREQUAL sha256)
      return()
    endif()
  endif()
  message(STATUS "Downloading ${url}")
  file(DOWNLOAD "${url}" "${destination}" EXPECTED_HASH SHA256=${sha256} STATUS _status TLS_VERIFY ON)
  list(GET _status 0 _code)
  if(NOT _code EQUAL 0)
    file(REMOVE "${destination}")
    message(FATAL_ERROR "Download failed (${_status}): ${url}")
  endif()
endfunction()

# Khronos headers -> deps/gl/{EGL,GLES2,GLES3,KHR}
set(OPENFRENZY_GL_INCLUDE "${OPENFRENZY_DEPS_DIR}/gl")
string(JSON _count LENGTH "${_of_manifest}" headers)
math(EXPR _last "${_count} - 1")
foreach(_i RANGE ${_last})
  string(JSON _path GET "${_of_manifest}" headers ${_i} path)
  string(JSON _url GET "${_of_manifest}" headers ${_i} url)
  string(JSON _sha GET "${_of_manifest}" headers ${_i} sha256)
  _of_download("${_url}" "${_sha}" "${OPENFRENZY_GL_INCLUDE}/${_path}")
endforeach()

# ANGLE xcframeworks -> deps/angle/dist
string(JSON _url GET "${_of_manifest}" angle url)
string(JSON _sha GET "${_of_manifest}" angle sha256)
set(_zip "${OPENFRENZY_DEPS_DIR}/angle/angle.zip")
_of_download("${_url}" "${_sha}" "${_zip}")
set(OPENFRENZY_ANGLE_DIR "${OPENFRENZY_DEPS_DIR}/angle/dist" CACHE PATH "ANGLE dist directory")
if(NOT EXISTS "${OPENFRENZY_ANGLE_DIR}/BUILD_INFO.txt")
  file(ARCHIVE_EXTRACT INPUT "${_zip}" DESTINATION "${OPENFRENZY_DEPS_DIR}/angle")
endif()
set(OPENFRENZY_EGL_FRAMEWORKS "${OPENFRENZY_ANGLE_DIR}/EGL.xcframework/macos-arm64")
set(OPENFRENZY_GLES_FRAMEWORKS "${OPENFRENZY_ANGLE_DIR}/GLESv2.xcframework/macos-arm64")
foreach(_lib "${OPENFRENZY_EGL_FRAMEWORKS}/libEGL.framework/libEGL"
             "${OPENFRENZY_GLES_FRAMEWORKS}/libGLESv2.framework/libGLESv2")
  if(NOT EXISTS "${_lib}")
    message(FATAL_ERROR "ANGLE is incomplete: ${_lib} is missing")
  endif()
endforeach()
# ANGLE's EGL loads libGLESv2.dylib from its own directory (as in OpenCE tools/macos_build.py).
set(_companion "${OPENFRENZY_EGL_FRAMEWORKS}/libEGL.framework/libGLESv2.dylib")
if(NOT EXISTS "${_companion}")
  file(CREATE_LINK "${OPENFRENZY_GLES_FRAMEWORKS}/libGLESv2.framework/libGLESv2" "${_companion}"
       SYMBOLIC)
endif()
