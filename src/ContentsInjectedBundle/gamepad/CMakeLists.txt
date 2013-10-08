pkg_check_modules(UDEV REQUIRED udev)
pkg_check_modules(GIO REQUIRED gio-unix-2.0)

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${UDEV_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
)

set(gamepad_SOURCES
  BrowserPlatformGamepad.cpp
  Gamepad.cpp
)

set(gamepad_LIBRARIES
  ${WebKitNix_LIBRARIES}
  ${UDEV_LIBRARIES}
  ${GIO_LIBRARIES}
  udev
)

add_library(gamepad STATIC ${gamepad_SOURCES})
target_link_libraries(gamepad ${gamepad_LIBRARIES})
set_target_properties(gamepad PROPERTIES COMPILE_FLAGS "-fPIC")
