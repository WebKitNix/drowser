pkg_check_modules(GSTREAMER gstreamer-0.10)
pkg_check_modules(GSTREAMER-APP gstreamer-app-0.10)
pkg_check_modules(GSTREAMER-AUDIO gstreamer-audio-0.10)

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER-AUDIO_INCLUDE_DIRS}
  ${GSTREAMER-APP_INCLUDE_DIRS}
)

link_directories(
  ${WebKitNix_LIBRARY_DIRS}
  ${GSTREAMER_LIBRARY_DIRS}
  ${GSTREAMER-APP_LIBRARY_DIRS}
  ${GSTREAMER-AUDIO_LIBRARY_DIRS}
)

set(audio_SOURCES
  AudioDestination.cpp
  AudioFileReader.cpp
  PlatformClientAudio.cpp
  WebKitWebAudioSourceGStreamer.cpp
)

set(audio_LIBRARIES
  ${WebKitNix_LIBRARIES}
  ${GSTREAMER_LIBRARIES}
  ${GSTREAMER-APP_LIBRARIES}
  ${GSTREAMER-AUDIO_LIBRARIES}
)

add_library(audio STATIC ${audio_SOURCES})
target_link_libraries(audio ${audio_LIBRARIES})
set_target_properties(audio PROPERTIES COMPILE_FLAGS "-fPIC")
