include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(audio)
add_subdirectory(gamepad)
add_subdirectory(mediaplayer)

set(PageBundle_SOURCES
  PageBundle.cpp
  BrowserPlatform.cpp
)

set(PageBundle_LIBRARIES
  ${WebKitNix_LIBRARIES}
  audio
  gamepad
  mediaplayer
)

add_library(PageBundle SHARED ${PageBundle_SOURCES})
target_link_libraries(PageBundle ${PageBundle_LIBRARIES})
