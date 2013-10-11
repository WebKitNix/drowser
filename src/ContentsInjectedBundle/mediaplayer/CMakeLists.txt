pkg_check_modules(GSTREAMER gstreamer-1.0 REQUIRED)
pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER-APP_INCLUDE_DIRS}
  ${GSTREAMER-AUDIO_INCLUDE_DIRS}
  ${GSTREAMER-PBUTILS_INCLUDE_DIRS}
)

link_directories(
  ${WebKitNix_LIBRARY_DIRS}
  ${GSTREAMER_LIBRARY_DIRS}
  ${GSTREAMER-APP_LIBRARY_DIRS}
  ${GSTREAMER-AUDIO_LIBRARY_DIRS}
  ${GSTREAMER-PBUTILS_LIBRARY_DIRS}
)

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
)

set(mediaplayer_SOURCES
  MediaPlayer.cpp
)

set(mediaplayer_LIBRARIES
  ${WebKitNix_LIBRARIES}
)

add_library(mediaplayer STATIC ${mediaplayer_SOURCES})
target_link_libraries(mediaplayer ${mediaplayer_LIBRARIES})
set_target_properties(mediaplayer PROPERTIES COMPILE_FLAGS "-fPIC")
