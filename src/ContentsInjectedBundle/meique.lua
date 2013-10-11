addSubdirectory("audio")
addSubdirectory("gamepad")
addSubdirectory("mediaplayer")

pageBundle = Library:new("PageBundle")
pageBundle:usePackage(nix)
pageBundle:useTarget(audio)
pageBundle:useTarget(gamepad)
pageBundle:useTarget(mediaPlayer)

pageBundle:addFiles([[
    PageBundle.cpp
    BrowserPlatform.cpp
]])
