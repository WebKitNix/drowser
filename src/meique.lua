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
  Bossa.cpp
  DesktopWindow.cpp
]])

UNIX:bossa:addFiles([[
  x11/DesktopWindowLinux.cpp
  x11/XlibEventSource.cpp
]])

copyFile("ui.html")
bossaUi = CustomTarget:new("ui", function()
    os.execute("ln -sd "..bossaUi:sourceDir().."images "..bossaUi:buildDir().." 2> /dev/null")
end)
bossa:addDependency(bossaUi)

addSubdirectory("UiBundle")
