
gio = findPackage("gio-unix-2.0", REQUIRED)
udev = findPackage("libudev", REQUIRED)

gamepad = Library:new("gamepad", STATIC)
gamepad:usePackage(gio)
gamepad:usePackage(udev)
gamepad:usePackage(nix)
gamepad:addFile("Gamepad.cpp")
