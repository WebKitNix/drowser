glib = findPackage("glib-2.0", REQUIRED)
openGL = findPackage("gl", REQUIRED)
x11 = findPackage("x11", REQUIRED)
cairo = findPackage("cairo", REQUIRED)
nix = findPackage("WebKitNix", REQUIRED)

bossa = Executable:new("bossa")
bossa:usePackage(glib)
bossa:usePackage(openGL)
bossa:usePackage(x11)
bossa:usePackage(cairo)
bossa:usePackage(nix)

bossa:addFiles([[
  main.cpp
  Browser.cpp
  DesktopWindow.cpp
  InjectedBundleGlue.cpp
]])

UNIX:bossa:addFiles([[
  x11/DesktopWindowLinux.cpp
  x11/XlibEventSource.cpp
]])

copyFile("ui.html")
copyFile("jquery-1.8.3.min.js")
copyFile("style.css")
bossaUi = CustomTarget:new("ui", function()
    os.execute("ln -sd "..bossaUi:sourceDir().."images "..bossaUi:buildDir().." 2> /dev/null")
end)
bossa:addDependency(bossaUi)
bossa:addCustomFlags("-std=c++11 -D'UI_SEARCH_PATH=\""..bossa:sourceDir().."ui\"'")
bossa:install("bin")

addSubdirectory("UiBundle")
-- meique doesn't support targetless installs yet.
-- addSubdirectory("ui")
