pkg_check_modules(GSTREAMER gstreamer-1.0 REQUIRED)
pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)
pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)

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
  AudioFileReader.cpp
  AudioLiveInputPipeline.cpp
  BrowserPlatformAudio.cpp
  FFTGStreamer.cpp
  GstAudioDevice.cpp
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
