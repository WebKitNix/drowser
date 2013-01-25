include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
