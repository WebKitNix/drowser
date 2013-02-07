addSubdirectory("audio")
-- addSubdirectory("gamepad")

pageBundle = Library:new("PageBundle")
pageBundle:usePackage(nix)
pageBundle:useTarget(audio)
-- pageBundle:useTarget(gamepad)

-- meique bug, it doesn't remember the linker libraries of static libraries
pageBundle:usePackage(gio)
pageBundle:usePackage(udev)

pageBundle:addFiles("PageBundle.cpp")
