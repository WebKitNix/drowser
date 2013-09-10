set(GSTREAMER_MINIMUM_VERSION 1.0.5)

pkg_check_modules(GST1_TEST gstreamer-1.0)
if ( GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
    pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
    pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)
    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)
    add_definitions(-DGST_API_VERSION_1=1)
else()
    # fallback to gstreamer-0.10
    unset(GSTREAMER_MINIMUM_VERSION)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-0.10)
    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-0.10)
    pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-0.10)
    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-0.10)
    set_source_files_properties(WebKitWebAudioSourceGStreamer.cpp PROPERTIES COMPILE_DEFINITIONS "GLIB_DISABLE_DEPRECATION_WARNINGS=1")
endif()

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER-APP_INCLUDE_DIRS}
  ${GSTREAMER-AUDIO_INCLUDE_DIRS}
  ${GSTREAMER-PBUTILS_INCLUDE_DIRS}
  ${GSTREAMER-FFT_INCLUDE_DIRS}
)

link_directories(
  ${WebKitNix_LIBRARY_DIRS}
  ${GSTREAMER_LIBRARY_DIRS}
  ${GSTREAMER-APP_LIBRARY_DIRS}
  ${GSTREAMER-AUDIO_LIBRARY_DIRS}
  ${GSTREAMER-PBUTILS_LIBRARY_DIRS}
  ${GSTREAMER-FFT_LIBRARY_DIRS}
)

set(audio_SOURCES
  GstAudioDevice.cpp
  AudioFileReader.cpp
  FFTGStreamer.cpp
  PlatformClientAudio.cpp
  WebKitWebAudioSourceGStreamer.cpp
)

set(audio_LIBRARIES
  ${WebKitNix_LIBRARIES}
  ${GSTREAMER_LIBRARIES}
  ${GSTREAMER-APP_LIBRARIES}
  ${GSTREAMER-AUDIO_LIBRARIES}
  ${GSTREAMER-PBUTILS_LIBRARIES}
  ${GSTREAMER-FFT_LIBRARIES}
)

add_library(audio STATIC ${audio_SOURCES})
target_link_libraries(audio ${audio_LIBRARIES})
set_target_properties(audio PROPERTIES COMPILE_FLAGS "-fPIC")
