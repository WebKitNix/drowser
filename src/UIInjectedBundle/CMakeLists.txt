set(UiBundle_SOURCES
  Bundle.cpp
)

set(UiBundle_LIBRARIES
  ${WebKitNix_LIBRARIES}
)

add_library(UiBundle SHARED ${UiBundle_SOURCES})
target_link_libraries(UiBundle ${UiBundle_LIBRARIES})
