# Makes OpenFrenzy.app from the host, the guest image and ANGLE (target openfrenzy_app).
# Layout and install-name steps follow ref/OpenCE-mac-innerhat tools/macos_build.py
# (package_into, CC0): ANGLE's two frameworks become plain dylibs in Contents/Frameworks.
# The bundle holds no game data: the player's disc image stays outside it (docs/BUILD.md).
# Usage: cmake -DAPP=... -DHOST=... -DGUEST=... -DPLIST=... -DEGL_DIR=... -DGLES_DIR=... -P make_app.cmake

file(REMOVE_RECURSE "${APP}")
set(_contents "${APP}/Contents")
file(MAKE_DIRECTORY "${_contents}/MacOS" "${_contents}/Resources" "${_contents}/Frameworks")
set(_exe "${_contents}/MacOS/openfrenzy")
file(COPY_FILE "${HOST}" "${_exe}")
file(COPY_FILE "${GUEST}" "${_contents}/Resources/openfrenzy_guest.elf")
file(COPY_FILE "${PLIST}" "${_contents}/Info.plist")

function(_run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _out)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "${ARGN} failed (${_result}):\n${_out}")
  endif()
endfunction()

# ANGLE: libEGL loads libGLESv2.dylib from its own directory.
foreach(_pair "EGL;${EGL_DIR}" "GLESv2;${GLES_DIR}")
  list(GET _pair 0 _name)
  list(GET _pair 1 _dir)
  set(_lib "${_contents}/Frameworks/lib${_name}.dylib")
  file(COPY_FILE "${_dir}/lib${_name}.framework/lib${_name}" "${_lib}")
  _run(install_name_tool -id "@rpath/lib${_name}.dylib" "${_lib}")
  _run(install_name_tool -change "@rpath/lib${_name}.framework/lib${_name}" "@rpath/lib${_name}.dylib" "${_exe}")
  _run(install_name_tool -delete_rpath "${_dir}" "${_exe}")
endforeach()
_run(install_name_tool -add_rpath "@executable_path/../Frameworks" "${_exe}")

# Ad-hoc signature: the dylibs first, then the bundle (install_name_tool broke the old ones).
foreach(_lib libEGL.dylib libGLESv2.dylib)
  _run(codesign --force --sign - "${_contents}/Frameworks/${_lib}")
endforeach()
_run(codesign --force --sign - "${APP}")
_run(codesign --verify --deep --strict "${APP}")
