glib = findPackage("glib-2.0", REQUIRED)
openGL = findPackage("gl", REQUIRED)
x11 = findPackage("x11", REQUIRED)
nix = findPackage("WebKitNix", REQUIRED)

browser = Executable:new("browser")
browser:usePackage(glib)
browser:usePackage(openGL)
browser:usePackage(x11)
browser:usePackage(nix)

browser:addFiles([[
  main.cpp
  Browser.cpp
  DesktopWindow.cpp
  InjectedBundleGlue.cpp
  Tab.cpp
]])

UNIX:browser:addFiles([[
  x11/DesktopWindowLinux.cpp
  x11/XlibEventSource.cpp
]])

copyFile("ui.html")
copyFile("jquery-1.8.3.min.js")
copyFile("style.css")
browserUi = CustomTarget:new("ui", function()
    os.execute("ln -sd "..browserUi:sourceDir().."images "..browserUi:buildDir().." 2> /dev/null")
end)
browser:addDependency(browserUi)
browser:addCustomFlags("-Wall -std=c++11 -D'UI_SEARCH_PATH=\""..browser:sourceDir().."ui\"'")
browser:install("bin")

addSubdirectory("UiBundle")
-- meique doesn't support targetless installs yet.
-- addSubdirectory("ui")
