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

# musl (the guest C library, as in OpenCE tools/android_build.py) -> deps/musl-<version>
string(JSON _url GET "${_of_manifest}" musl url)
string(JSON _sha GET "${_of_manifest}" musl sha256)
string(JSON _ver GET "${_of_manifest}" musl version)
_of_download("${_url}" "${_sha}" "${OPENFRENZY_DEPS_DIR}/musl-${_ver}.tar.gz")
set(OPENFRENZY_MUSL_DIR "${OPENFRENZY_DEPS_DIR}/musl-${_ver}" CACHE PATH "musl source directory")
set(OPENFRENZY_MUSL_VERSION "${_ver}")
if(NOT EXISTS "${OPENFRENZY_MUSL_DIR}/COPYRIGHT")
  file(ARCHIVE_EXTRACT INPUT "${OPENFRENZY_DEPS_DIR}/musl-${_ver}.tar.gz" DESTINATION "${OPENFRENZY_DEPS_DIR}")
endif()

# FFmpeg for the Bink videos (port/macos/host/host_bink.c), built from the pinned source archive
# -> deps/ffmpeg-<version>-<key>. LGPL only: no GPL, no nonfree, no version3 code, no external
# library; only the Bink demuxer and decoders, libavformat, libavcodec, libswscale and libavutil
# (shared, so a user can replace them). <key> is a hash of the tarball and the configure line, so
# a changed line gives a new build and an unchanged one is never built again. Called from
# CMakeLists.txt when OPENFRENZY_BINK_FFMPEG is on. Sets OPENFRENZY_FFMPEG_DIR (include, lib,
# LICENSES) and OPENFRENZY_FFMPEG_VERSION. Needs only Apple clang and make (no nasm on arm64).
function(openfrenzy_ffmpeg)
  string(JSON _url GET "${_of_manifest}" ffmpeg url)
  string(JSON _sha GET "${_of_manifest}" ffmpeg sha256)
  string(JSON _ver GET "${_of_manifest}" ffmpeg version)
  # The app's minimum macOS (Info.plist), unless the build sets one.
  set(_min "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  if(NOT _min)
    file(READ "${CMAKE_SOURCE_DIR}/port/macos/Info.plist" _plist)
    string(REGEX MATCH "<key>LSMinimumSystemVersion</key>[ \t\r\n]*<string>([0-9.]+)</string>" _ "${_plist}")
    set(_min "${CMAKE_MATCH_1}")
  endif()
  # --prefix is only a name (make install DESTDIR=...): the configuration string that the
  # libraries keep (avcodec_configuration) holds no local path.
  set(_configure
    --prefix=/usr/local --cc=/usr/bin/clang --arch=aarch64 --target-os=darwin
    --install-name-dir=@rpath
    --extra-cflags=-mmacosx-version-min=${_min} --extra-ldflags=-mmacosx-version-min=${_min}
    --disable-everything --disable-gpl --disable-nonfree --disable-version3
    --disable-programs --disable-doc --disable-network --disable-autodetect
    --enable-shared --disable-static
    --disable-avdevice --disable-avfilter --disable-swresample
    --enable-demuxer=bink
    --enable-decoder=bink --enable-decoder=binkaudio_rdft --enable-decoder=binkaudio_dct)
  string(SHA256 _key "${_sha};${_configure}")
  string(SUBSTRING "${_key}" 0 12 _key)
  set(_prefix "${OPENFRENZY_DEPS_DIR}/ffmpeg-${_ver}-${_key}")
  set(OPENFRENZY_FFMPEG_DIR "${_prefix}" PARENT_SCOPE)
  set(OPENFRENZY_FFMPEG_VERSION "${_ver}" PARENT_SCOPE)
  if(EXISTS "${_prefix}/BUILD_OK")
    return()
  endif()

  # One build at a time: build directories share deps/.
  file(LOCK "${OPENFRENZY_DEPS_DIR}/ffmpeg.lock" GUARD FUNCTION TIMEOUT 1800)
  if(EXISTS "${_prefix}/BUILD_OK")
    return()
  endif()
  set(_tarball "${OPENFRENZY_DEPS_DIR}/ffmpeg-${_ver}.tarball")
  _of_download("${_url}" "${_sha}" "${_tarball}")
  # Source and build tree in <prefix>.build, removed after a good build.
  set(_work "${_prefix}.build")
  set(_source "${_work}/source")
  file(REMOVE_RECURSE "${_work}" "${_prefix}")
  file(ARCHIVE_EXTRACT INPUT "${_tarball}" DESTINATION "${_work}/extract")
  file(GLOB _top "${_work}/extract/*")  # (one top folder; its name does not matter)
  file(RENAME "${_top}" "${_source}")
  file(MAKE_DIRECTORY "${_work}/build")
  find_program(OPENFRENZY_MAKE make REQUIRED)
  cmake_host_system_information(RESULT _jobs QUERY NUMBER_OF_LOGICAL_CORES)
  message(STATUS "Building FFmpeg ${_ver} (LGPL, Bink only) in ${_prefix} (log: ${_work}/build.log)")
  # GIT_CEILING_DIRECTORIES: deps/ is inside this repository; FFmpeg's version.sh must not
  # take this repository's git description as its version.
  foreach(_step "${_source}/configure;${_configure}"
                "${OPENFRENZY_MAKE};-j${_jobs}"
                "${OPENFRENZY_MAKE};install;DESTDIR=${_work}/stage")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E env "GIT_CEILING_DIRECTORIES=${OPENFRENZY_DEPS_DIR}" ${_step}
      WORKING_DIRECTORY "${_work}/build" RESULT_VARIABLE _result
      OUTPUT_FILE "${_work}/step.log" ERROR_FILE "${_work}/step.log")
    file(READ "${_work}/step.log" _log)
    file(APPEND "${_work}/build.log" "${_log}")
    if(NOT _result EQUAL 0)
      message(FATAL_ERROR "FFmpeg build step failed (${_result}): ${_step}\nLog: ${_work}/build.log")
    endif()
  endforeach()
  file(RENAME "${_work}/stage/usr/local" "${_prefix}")

  # Licence files and the source note, copied into the app by cmake/bundle_dylibs.py.
  file(MAKE_DIRECTORY "${_prefix}/LICENSES")
  file(COPY "${_source}/COPYING.LGPLv2.1" "${_source}/LICENSE.md" DESTINATION "${_prefix}/LICENSES")
  list(JOIN _configure " " _line)
  file(WRITE "${_prefix}/LICENSES/SOURCE.txt"
"FFmpeg ${_ver} in OpenFrenzy.app

The app uses FFmpeg only to play the game's Bink videos. The FFmpeg libraries in
Contents/Frameworks (libavformat, libavcodec, libswscale, libavutil) are licensed under the
GNU Lesser General Public License, version 2.1 or later (COPYING.LGPLv2.1, LICENSE.md).
This build has no GPL, nonfree or version 3 part and no external library.

Source code (the exact archive this build used, not changed):
  ${_url}
  SHA-256 ${_sha}

Configure line (then make and make install):
  ./configure ${_line}

The libraries are dynamic libraries. The app loads them from Contents/Frameworks by their
install names (@rpath/libavformat.<major>.dylib and so on). To use a changed FFmpeg: build
it from the source above with the configure line above, copy your dylibs over the ones in
Contents/Frameworks (the same file names), and sign the app again, for example:
  codesign --force --options runtime --sign - OpenFrenzy.app/Contents/Frameworks/*.dylib
  codesign --force --options runtime --entitlements OpenFrenzy.entitlements --sign - OpenFrenzy.app
(OpenFrenzy.entitlements is port/macos/OpenFrenzy.entitlements in the OpenFrenzy source.)
")
  file(WRITE "${_prefix}/PACKAGE" "ffmpeg ${_ver}\n")
  file(REMOVE_RECURSE "${_work}")
  file(WRITE "${_prefix}/BUILD_OK" "${_line}\n")
endfunction()
