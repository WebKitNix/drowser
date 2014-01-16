gstreamer = findPackage("gstreamer-1.0", REQUIRED)
gstreamerApp = findPackage("gstreamer-app-1.0", REQUIRED)
gstreamerAudio = findPackage("gstreamer-audio-1.0", REQUIRED)
gstreamerFft = findPackage("gstreamer-fft-1.0", REQUIRED)
gstreamPbUtils = findPackage("gstreamer-pbutils-1.0", REQUIRED)

addSubdirectory("audio")
addSubdirectory("gamepad")
addSubdirectory("mediaplayer")
addSubdirectory("mediastream")

pageBundle = Library:new("PageBundle")
pageBundle:use(nix)
pageBundle:use(audio)
pageBundle:use(gamepad)
pageBundle:use(mediaPlayer)
pageBundle:use(mediastream)

pageBundle:addFiles([[
    PageBundle.cpp
    BrowserPlatform.cpp
]])
