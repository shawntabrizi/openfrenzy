# FFmpeg for the web build's Bink videos (port/macos/host/host_bink.c, compiled for wasm):
# the same pinned source (port/macos/dependencies.json) and the same LGPL, Bink-only choice
# as the native build (cmake/Dependencies.cmake openfrenzy_ffmpeg), built with Emscripten as
# static libraries -> deps/ffmpeg-web-<version>-<key>. No assembly (WebAssembly has none
# FFmpeg knows), no threads of its own (Bink decodes on the caller's thread), compiled with
# -pthread because the module uses shared memory. Sets OPENFRENZY_WEB_FFMPEG_DIR.

set(OPENFRENZY_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps" CACHE PATH "Downloaded build dependencies")

function(openfrenzy_web_ffmpeg)
  file(READ "${CMAKE_SOURCE_DIR}/port/macos/dependencies.json" _manifest)
  string(JSON _url GET "${_manifest}" ffmpeg url)
  string(JSON _sha GET "${_manifest}" ffmpeg sha256)
  string(JSON _ver GET "${_manifest}" ffmpeg version)
  get_filename_component(_em_bin "${OPENFRENZY_EMCC}" DIRECTORY)
  set(_configure
    --prefix=/usr/local
    --cc=${_em_bin}/emcc --cxx=${_em_bin}/em++ --ar=${_em_bin}/emar --ranlib=${_em_bin}/emranlib
    --nm=${_em_bin}/emnm
    --target-os=none --arch=x86_32 --enable-cross-compile
    --disable-asm --disable-inline-asm --disable-x86asm --disable-stripping
    --disable-pthreads --disable-w32threads --disable-os2threads
    "--extra-cflags=-pthread -O2"
    --disable-everything --disable-gpl --disable-nonfree --disable-version3
    --disable-programs --disable-doc --disable-network --disable-autodetect
    --enable-static --disable-shared
    --disable-avdevice --disable-avfilter --disable-swresample
    --enable-demuxer=bink
    --enable-decoder=bink --enable-decoder=binkaudio_rdft --enable-decoder=binkaudio_dct)
  string(SHA256 _key "${_sha};${_configure}")
  string(SUBSTRING "${_key}" 0 12 _key)
  set(_prefix "${OPENFRENZY_DEPS_DIR}/ffmpeg-web-${_ver}-${_key}")
  set(OPENFRENZY_WEB_FFMPEG_DIR "${_prefix}" PARENT_SCOPE)
  if(EXISTS "${_prefix}/BUILD_OK")
    return()
  endif()

  file(LOCK "${OPENFRENZY_DEPS_DIR}/ffmpeg-web.lock" GUARD FUNCTION TIMEOUT 1800)
  if(EXISTS "${_prefix}/BUILD_OK")
    return()
  endif()
  set(_tarball "${OPENFRENZY_DEPS_DIR}/ffmpeg-${_ver}.tarball")
  set(_have "")
  if(EXISTS "${_tarball}")
    file(SHA256 "${_tarball}" _have)
  endif()
  if(NOT _have STREQUAL _sha)
    message(STATUS "Downloading ${_url}")
    file(DOWNLOAD "${_url}" "${_tarball}" EXPECTED_HASH SHA256=${_sha} STATUS _status TLS_VERIFY ON)
    list(GET _status 0 _code)
    if(NOT _code EQUAL 0)
      file(REMOVE "${_tarball}")
      message(FATAL_ERROR "Download failed (${_status}): ${_url}")
    endif()
  endif()
  set(_work "${_prefix}.build")
  set(_source "${_work}/source")
  file(REMOVE_RECURSE "${_work}" "${_prefix}")
  file(ARCHIVE_EXTRACT INPUT "${_tarball}" DESTINATION "${_work}/extract")
  file(GLOB _top "${_work}/extract/*")
  file(RENAME "${_top}" "${_source}")
  file(MAKE_DIRECTORY "${_work}/build")
  find_program(OPENFRENZY_MAKE make REQUIRED)
  cmake_host_system_information(RESULT _jobs QUERY NUMBER_OF_LOGICAL_CORES)
  message(STATUS "Building FFmpeg ${_ver} for WebAssembly (LGPL, Bink only) in ${_prefix} (log: ${_work}/build.log)")
  set(_env "GIT_CEILING_DIRECTORIES=${OPENFRENZY_DEPS_DIR}" "EM_CACHE=${OPENFRENZY_EM_CACHE}")
  if(OPENFRENZY_EM_PYTHON)
    list(APPEND _env "EMSDK_PYTHON=${OPENFRENZY_EM_PYTHON}")
  endif()
  foreach(_step "${_source}/configure;${_configure}"
                "${OPENFRENZY_MAKE};-j${_jobs}"
                "${OPENFRENZY_MAKE};install;DESTDIR=${_work}/stage")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E env ${_env} ${_step}
      WORKING_DIRECTORY "${_work}/build" RESULT_VARIABLE _result
      OUTPUT_FILE "${_work}/step.log" ERROR_FILE "${_work}/step.log")
    file(READ "${_work}/step.log" _log)
    file(APPEND "${_work}/build.log" "${_log}")
    if(NOT _result EQUAL 0)
      message(FATAL_ERROR "FFmpeg (web) build step failed (${_result}): ${_step}\nLog: ${_work}/build.log")
    endif()
  endforeach()
  file(RENAME "${_work}/stage/usr/local" "${_prefix}")
  file(MAKE_DIRECTORY "${_prefix}/LICENSES")
  file(COPY "${_source}/COPYING.LGPLv2.1" "${_source}/LICENSE.md" DESTINATION "${_prefix}/LICENSES")
  list(JOIN _configure " " _line)
  string(REPLACE "${_em_bin}/" "" _line "${_line}")
  file(WRITE "${_prefix}/LICENSES/SOURCE.txt"
"FFmpeg ${_ver} in the OpenFrenzy web build

The web build uses FFmpeg only to play the game's Bink videos. The FFmpeg code in
openfrenzy.wasm (libavformat, libavcodec, libswscale, libavutil) is licensed under the GNU
Lesser General Public License, version 2.1 or later (COPYING.LGPLv2.1, LICENSE.md). This build
has no GPL, nonfree or version 3 part and no external library.

Source code (the exact archive this build used, not changed):
  ${_url}
  SHA-256 ${_sha}

Configure line (Emscripten's emcc, emar, emranlib and emnm; then make and make install):
  ./configure ${_line}

The libraries are linked statically into openfrenzy.wasm. To use a changed FFmpeg, build it
with the line above and build the web target again (docs/WEB.md); its source and build files
are in the OpenFrenzy repository.
")
  file(REMOVE_RECURSE "${_work}")
  file(WRITE "${_prefix}/BUILD_OK" "${_line}\n")
endfunction()
