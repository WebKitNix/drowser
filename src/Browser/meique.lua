browser = Executable:new("drowser")
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

browser:addCustomFlags("-Wall -std=c++0x -D'UI_SEARCH_PATH=\""..browser:sourceDir().."ui\"'")

-- Install routines
browser:install("bin")
browser:install([[
  ui/jquery-1.8.3.min.js
  ui/jquery.hotkeys.js
  ui/style.css
  ui/ui.html
]], "share/Drowser")
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
]], "share/Drowser/images")
