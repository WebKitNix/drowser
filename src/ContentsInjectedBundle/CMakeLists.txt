add_subdirectory(gamepad)

set(PageBundle_SOURCES
  PageBundle.cpp
)

set(PageBundle_LIBRARIES
  ${WebKitNix_LIBRARIES}
  gamepad
)

add_library(PageBundle SHARED ${PageBundle_SOURCES})
target_link_libraries(PageBundle ${PageBundle_LIBRARIES})
