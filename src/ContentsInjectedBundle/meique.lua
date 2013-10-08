addSubdirectory("audio")
addSubdirectory("gamepad")

pageBundle = Library:new("PageBundle")
pageBundle:usePackage(nix)
pageBundle:useTarget(audio)
pageBundle:useTarget(gamepad)

pageBundle:addFiles([[
    PageBundle.cpp
    BrowserPlatform.cpp
]])
