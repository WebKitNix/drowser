find_package(PkgConfig REQUIRED)

pkg_check_modules(WebKitNix REQUIRED WebKitNix)
pkg_check_modules(GLIB REQUIRED glib-2.0)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(
  ${WebKitNix_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${X11_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}

  ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
  ${WebKitNix_LIBRARY_DIRS}
  ${GLIB_LIBRARY_DIRS}
)

set(drowser_LIBRARIES
  ${WebKitNix_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${X11_LIBRARIES}
  ${OPENGL_LIBRARIES}
)

set(drowser_SOURCES
  main.cpp
  Browser.cpp
  DesktopWindow.cpp
  InjectedBundleGlue.cpp
  Tab.cpp

  x11/DesktopWindowLinux.cpp
  x11/XlibEventSource.cpp
)

add_definitions(-DUI_SEARCH_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ui\")

add_executable(drowser ${drowser_SOURCES})
target_link_libraries(drowser ${drowser_LIBRARIES})

add_subdirectory(injectedbundle/ui)
add_subdirectory(injectedbundle/page)
