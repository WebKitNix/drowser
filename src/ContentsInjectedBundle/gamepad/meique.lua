
gio = findPackage("gio-unix-2.0", REQUIRED)
udev = findPackage("libudev", REQUIRED)

gamepad = Library:new("gamepad", STATIC)
gamepad:addCustomFlags("-std=c++0x")
gamepad:usePackage(gio)
gamepad:usePackage(udev)
gamepad:usePackage(nix)
gamepad:addIncludePath("..")
gamepad:addFiles([[
    Gamepad.cpp
    BrowserPlatformGamepad.cpp
]])
