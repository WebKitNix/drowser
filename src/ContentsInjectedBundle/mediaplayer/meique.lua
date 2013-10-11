gstreamer = findPackage("gstreamer-1.0", REQUIRED)
gstreamerApp = findPackage("gstreamer-app-1.0", REQUIRED)
gstreamerAudio = findPackage("gstreamer-audio-1.0", REQUIRED)
gstreamerFft = findPackage("gstreamer-fft-1.0", REQUIRED)
gstreamPbUtils = findPackage("gstreamer-pbutils-1.0", REQUIRED)

mediaPlayer = Library:new("mediaPlayer", STATIC)
mediaPlayer:addFiles([[
    MediaPlayer.cpp
]])

mediaPlayer:addIncludePath("..")
mediaPlayer:usePackage(gstreamer)
mediaPlayer:usePackage(gstreamerApp)
mediaPlayer:usePackage(gstreamerAudio)
mediaPlayer:usePackage(gstreamerFft)
mediaPlayer:usePackage(gstreamPbUtils)
mediaPlayer:usePackage(nix)

mediaPlayer:addCustomFlags("-std=c++0x")
