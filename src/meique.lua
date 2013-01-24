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
browser:addCustomFlags("-Wall -std=c++0x -D'UI_SEARCH_PATH=\""..browser:sourceDir().."ui\"'")
browser:install("bin")
browser:install([[
  ui/jquery-1.8.3.min.js
  ui/jquery.hotkeys.js
  ui/style.css
  ui/ui.html
]], "share/Lasso")
-- Images
browser:install([[
  ui/images/btn_back.png
  ui/images/btn_forward.png
  ui/images/btn_refresh.png
  ui/images/plus.png
  ui/images/progressbar_center.png
  ui/images/progressbar_left.png
  ui/images/progressbar_right.png
  ui/images/tab_active_btn_close.png
  ui/images/tab_active_fill.png
  ui/images/tab_active_left.png
  ui/images/tab_active_right.png
  ui/images/tab_base_fill.png
  ui/images/tab_inactive_btn_close.png
  ui/images/tab_inactive_fill.png
  ui/images/tab_inactive_left.png
  ui/images/tab_inactive_right.png
  ui/images/urlbar_fill.png
  ui/images/urlbar_input_center.png
  ui/images/urlbar_input_left.png
  ui/images/urlbar_input_right.png
]], "share/Lasso/images")

addSubdirectory("injectedbundle")
