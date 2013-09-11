include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
)

set(mediastream_SOURCES
  MediaStreamCenter.cpp
  PlatformClientUserMedia.cpp
  UserMediaClient.cpp
)

set(mediastream_LIBRARIES
  ${WebKitNix_LIBRARIES}
)

add_library(mediastream STATIC ${mediastream_SOURCES})
target_link_libraries(mediastream ${mediastream_LIBRARIES})
set_target_properties(mediastream PROPERTIES COMPILE_FLAGS "-fPIC")
