
gio = findPackage("gio-unix-2.0", REQUIRED)
udev = findPackage("libudev", REQUIRED)

gamepad = Library:new("gamepad", STATIC)
gamepad:use(gio)
gamepad:use(udev)
gamepad:use(nix)
gamepad:addIncludePath("..")
gamepad:addFiles([[
    Gamepad.cpp
    BrowserPlatformGamepad.cpp
]])
