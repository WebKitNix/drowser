include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(audio)
add_subdirectory(gamepad)
add_subdirectory(usermedia)

set(PageBundle_SOURCES
  PageBundle.cpp
  PlatformClient.cpp
)

set(PageBundle_LIBRARIES
  ${WebKitNix_LIBRARIES}
  audio
  gamepad
  usermedia
)

add_library(PageBundle SHARED ${PageBundle_SOURCES})
target_link_libraries(PageBundle ${PageBundle_LIBRARIES})
