include_directories(
  ${WebKitNix_INCLUDE_DIRS}
)

set(usermedia_SOURCES
  PlatformClientUserMedia.cpp
  UserMediaClient.cpp
)

set(usermedia_LIBRARIES
  ${WebKitNix_LIBRARIES}
)

add_library(usermedia STATIC ${usermedia_SOURCES})
target_link_libraries(usermedia ${usermedia_LIBRARIES})
set_target_properties(usermedia PROPERTIES COMPILE_FLAGS "-fPIC")
