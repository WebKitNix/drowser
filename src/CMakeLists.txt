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
  "Shared"
)

link_directories(
  ${WebKitNix_LIBRARY_DIRS}
  ${GLIB_LIBRARY_DIRS}
)

add_subdirectory(Browser)
add_subdirectory(UIInjectedBundle)
# add_subdirectory(ContentsInjectedBundle)
