addSubdirectory("audio")
addSubdirectory("gamepad")
addSubdirectory("usermedia")

pageBundle = Library:new("PageBundle")
pageBundle:usePackage(nix)
pageBundle:useTarget(audio)
pageBundle:useTarget(gamepad)
pageBundle:useTarget(usermedia)

pageBundle:addFiles([[
    PageBundle.cpp
    PlatformClient.cpp
]])
