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
