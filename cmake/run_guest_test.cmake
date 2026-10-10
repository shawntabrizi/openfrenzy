# Runs one guest image in the host and checks that the guest itself exited with EXPECT.
# The host prints "Game exited (N)" only from host_exit, which the guest calls with main's
# return value; a crash, a loader failure or a timeout does not print it.
# Usage: cmake -DHOST=... -DIMAGE=... -DWORK=... -DEXPECT=N [-DISO=disc image] -P run_guest_test.cmake
file(MAKE_DIRECTORY "${WORK}/data" "${WORK}/saves")
set(ENV{OPENFRENZY_DATA_ROOT} "${WORK}/data")
set(ENV{OPENFRENZY_SAVE_ROOT} "${WORK}/saves")
# Headless: these images draw nothing, and the test must also run without a window server.
set(ENV{SDL_VIDEO_DRIVER} "dummy")
if(ISO)
  set(ENV{OPENFRENZY_ISO} "${ISO}")
endif()
execute_process(COMMAND "${HOST}" "${IMAGE}"
  RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out TIMEOUT 30)
message("${out}")
if(NOT result STREQUAL "${EXPECT}")
  message(FATAL_ERROR "host exit status '${result}', expected ${EXPECT}")
endif()
if(NOT out MATCHES "Game exited \\(${EXPECT}\\)")
  message(FATAL_ERROR "the guest did not report exit code ${EXPECT}")
endif()
if(NOT out MATCHES "Loaded ARM image 88000000-")
  message(FATAL_ERROR "the host did not report loading the guest image")
endif()
